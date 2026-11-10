#include "AnimatedTexture.h"
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace sse {
namespace {
constexpr std::size_t maxFileBytes = 8 * 1024 * 1024;
constexpr std::size_t maxDecodedBytes = 96 * 1024 * 1024;
constexpr unsigned maxDimension = 1024;
constexpr unsigned maxFrames = 512;

struct GifInfo {
    unsigned width = 0;
    unsigned height = 0;
    unsigned frames = 0;
};

std::vector<unsigned char> readGif(const std::filesystem::path& path) {
    // The filesystem overload keeps Windows paths containing Unicode intact.
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
        throw std::runtime_error("file could not be opened");
    const std::streamoff length = input.tellg();
    if (length < 13 || length > static_cast<std::streamoff>(maxFileBytes))
        throw std::runtime_error("file is incomplete or exceeds the 8 MiB limit");
    std::vector<unsigned char> bytes(static_cast<std::size_t>(length));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("file could not be read completely");
    return bytes;
}

// Count image descriptors before stb allocates all decoded frames. Skipping the
// length-prefixed data blocks avoids interpreting compressed pixels as markers.
GifInfo inspectGif(const std::vector<unsigned char>& bytes) {
    if (bytes.size() < 13 || bytes[0] != 'G' || bytes[1] != 'I' || bytes[2] != 'F' || bytes[3] != '8' ||
        (bytes[4] != '7' && bytes[4] != '9') || bytes[5] != 'a')
        throw std::runtime_error("invalid GIF signature");
    const auto word = [&](std::size_t position) -> unsigned {
        return unsigned(bytes[position]) | (unsigned(bytes[position + 1]) << 8);
    };
    GifInfo info{word(6), word(8), 0};
    if (!info.width || !info.height || info.width > maxDimension || info.height > maxDimension)
        throw std::runtime_error("GIF dimensions must be between 1 and 1024 pixels");
    // Each dimension has already been bounded, so this multiplication is safe.
    const std::size_t frameBytes = std::size_t(info.width) * info.height * 4;
    std::size_t cursor = 13;
    const auto skip = [&](std::size_t count) {
        if (count > bytes.size() - cursor)
            throw std::runtime_error("truncated GIF block");
        cursor += count;
    };
    const auto skipColorTable = [&](unsigned packed) {
        if (packed & 0x80)
            skip(std::size_t(3) * (std::size_t(2) << (packed & 7)));
    };
    const auto skipSubBlocks = [&]() {
        std::size_t total = 0;
        for (;;) {
            if (cursor == bytes.size())
                throw std::runtime_error("unterminated GIF data blocks");
            const unsigned count = bytes[cursor++];
            if (count == 0)
                return total;
            skip(count);
            total += count;
        }
    };
    skipColorTable(bytes[10]);
    while (cursor < bytes.size()) {
        const unsigned marker = bytes[cursor++];
        if (marker == 0x3b) {
            if (!info.frames)
                throw std::runtime_error("GIF contains no image frames");
            return info;
        }
        if (marker == 0x21) {
            if (cursor == bytes.size())
                throw std::runtime_error("truncated GIF extension");
            const unsigned label = bytes[cursor++];
            // stb treats the graphics-control extension specially. Require its
            // fixed layout so both parsers agree on the following image blocks.
            if (label == 0xf9) {
                if (bytes.size() - cursor < 6 || bytes[cursor] != 4 || bytes[cursor + 5] != 0)
                    throw std::runtime_error("invalid GIF graphics-control extension");
                // The bundled decoder has unsafe restore-previous (3) handling.
                // Reject it and reserved disposal modes before any frame decode.
                const unsigned disposal = (bytes[cursor + 1] >> 2) & 7;
                if (disposal >= 3)
                    throw std::runtime_error("unsupported GIF disposal method");
                cursor += 6;
                continue;
            }
            skipSubBlocks();
            continue;
        }
        if (marker != 0x2c)
            throw std::runtime_error("unexpected GIF block marker");
        if (bytes.size() - cursor < 9)
            throw std::runtime_error("truncated GIF image descriptor");
        const unsigned left = word(cursor);
        const unsigned top = word(cursor + 2);
        const unsigned width = word(cursor + 4);
        const unsigned height = word(cursor + 6);
        const unsigned packed = bytes[cursor + 8];
        cursor += 9;
        if (!width || !height || left + width > info.width || top + height > info.height)
            throw std::runtime_error("GIF image frame exceeds its canvas");
        ++info.frames;
        if (info.frames > maxFrames || info.frames > maxDecodedBytes / frameBytes)
            throw std::runtime_error("GIF exceeds the frame count or 96 MiB decoded limit");
        skipColorTable(packed);
        if (cursor == bytes.size() || bytes[cursor] < 2 || bytes[cursor] > 8)
            throw std::runtime_error("invalid GIF LZW code size");
        ++cursor;
        if (skipSubBlocks() == 0)
            throw std::runtime_error("GIF image contains no compressed data");
    }
    throw std::runtime_error("GIF trailer is missing");
}

class TextureUploadState {
  public:
    explicit TextureUploadState(GLuint texture) {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding_);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment_);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    ~TextureUploadState() {
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment_);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(binding_));
    }

  private:
    GLint binding_ = 0;
    GLint alignment_ = 4;
};
} // namespace

void AnimatedTexture::ImageDeleter::operator()(unsigned char* data) const noexcept {
    stbi_image_free(data);
}

AnimatedTexture::AnimatedTexture(const std::filesystem::path& path, bool monochrome) {
    try {
        const auto bytes = readGif(path);
        const GifInfo info = inspectGif(bytes);
        int width = 0, height = 0, frameCount = 0, channels = 0;
        int* rawDelays = nullptr;
        std::unique_ptr<unsigned char, ImageDeleter> pixels(stbi_load_gif_from_memory(
            bytes.data(), static_cast<int>(bytes.size()), &rawDelays, &width, &height, &frameCount,
            &channels, 4));
        // The bundled stb frees its delays internally on allocation failure,
        // without clearing the output pointer. Own it only after decode succeeds.
        std::unique_ptr<int, decltype(&stbi_image_free)> delays(pixels ? rawDelays : nullptr,
                                                             &stbi_image_free);
        if (!pixels || !delays) {
            const char* reason = stbi_failure_reason();
            throw std::runtime_error(reason ? reason : "GIF decoding failed");
        }
        if (width != static_cast<int>(info.width) || height != static_cast<int>(info.height) ||
            frameCount != static_cast<int>(info.frames))
            throw std::runtime_error("decoded GIF does not match its validated frames");
        if (monochrome) {
            const std::size_t totalBytes = std::size_t(info.width) * info.height * 4 * info.frames;
            for (std::size_t pixel = 0; pixel < totalBytes; pixel += 4) {
                const unsigned char value =
                    std::max({pixels.get()[pixel], pixels.get()[pixel + 1], pixels.get()[pixel + 2]});
                pixels.get()[pixel] = value;
                pixels.get()[pixel + 1] = value;
                pixels.get()[pixel + 2] = value;
            }
        }
        std::vector<double> frameEnds;
        frameEnds.reserve(info.frames);
        double duration = 0;
        for (int frame = 0; frame < frameCount; ++frame) {
            duration += delays.get()[frame] > 0 ? delays.get()[frame] / 1000.0 : 0.1;
            frameEnds.push_back(duration);
        }
        {
            const TextureUploadState state(texture_.id());
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         pixels.get());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        width_ = width;
        height_ = height;
        frameBytes_ = std::size_t(info.width) * info.height * 4;
        pixels_ = std::move(pixels);
        frameEnds_ = std::move(frameEnds);
        ready_ = true;
    } catch (const std::exception& error) {
        std::cerr << "Animated GIF unavailable: " << error.what() << '\n';
    }
}

void AnimatedTexture::update(double elapsedSeconds, bool motion) {
    if (!ready_ || !motion || !std::isfinite(elapsedSeconds))
        return;
    double position = std::fmod(elapsedSeconds, frameEnds_.back());
    if (position < 0)
        position += frameEnds_.back();
    if (position >= frameEnds_.back())
        position = 0; // Adding the period to a tiny negative value can round up.
    const auto end = std::upper_bound(frameEnds_.begin(), frameEnds_.end(), position);
    const std::size_t frame = static_cast<std::size_t>(end - frameEnds_.begin());
    if (frame == currentFrame_)
        return;
    const TextureUploadState state(texture_.id());
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE,
                    pixels_.get() + frame * frameBytes_);
    currentFrame_ = frame;
}
} // namespace sse
