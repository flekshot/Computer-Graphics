#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
namespace sse {
namespace {
std::vector<unsigned char> procedural(const std::string& kind, glm::vec3 tint, int w, int h) {
    std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            float u = float(x) / w, v = float(y) / h;
            float n = 0.5f + 0.19f * std::sin(u * 100 + std::sin(v * 43) * 3) +
                      0.12f * std::sin(v * 173 + u * 71) + 0.07f * std::cos(u * 259 - v * 103);
            glm::vec3 color = tint * (0.58f + n * 0.55f);
            float alpha = 1;
            if (kind == "Earth") {
                float land = std::sin(u * 24 + std::cos(v * 13) * 3) + 0.5f * std::cos(v * 37 - u * 8);
                color = land > 0.45f ? glm::vec3(0.18f, 0.36f, 0.16f) * (0.8f + n * 0.4f)
                                     : glm::vec3(0.025f, 0.15f, 0.35f);
                if (v < 0.07f || v > 0.93f)
                    color = glm::vec3(0.8f);
            }
            if (kind == "Jupiter" || kind == "Saturn" || kind == "Neptune" || kind == "Uranus")
                color = tint * (0.65f + 0.2f * std::sin(v * 105 + std::sin(u * 25) * 0.8f) + n * 0.2f);
            if (kind == "Sun")
                color = glm::vec3(1, 0.48f + n * 0.25f, 0.08f + n * 0.15f);
            if (kind == "Ring") {
                color = glm::vec3(0.78f, 0.70f, 0.54f) * (0.75f + 0.20f * std::sin(u * 450));
                alpha = 0.32f + 0.45f * (0.5f + 0.5f * std::sin(u * 160));
                if (u > 0.57f && u < 0.62f)
                    alpha = 0.025f;
            }
            if (kind == "Clouds")
                color = glm::vec3(std::max(0.0f, (n - 0.53f) * 2.1f));
            if (kind == "Night" || kind == "Sky")
                color = glm::vec3(0);
            const size_t i = (static_cast<size_t>(y) * w + x) * 4;
            for (int c = 0; c < 3; ++c)
                pixels[i + c] = static_cast<unsigned char>(std::clamp(color[c], 0.0f, 1.0f) * 255);
            pixels[i + 3] = static_cast<unsigned char>(alpha * 255);
        }
    return pixels;
}
} // namespace
Texture::Texture(const std::filesystem::path& path, const std::string& kind, glm::vec3 color, bool srgb) {
    int width = 0, height = 0, channels = 0;
    auto* data = stbi_load(path.u8string().c_str(), &width, &height, &channels, 4);
    std::vector<unsigned char> generated;
    if (!data) {
        fallback_ = true;
        width = 512;
        height = 256;
        generated = procedural(kind, color, width, height);
        std::cerr << "Texture fallback: " << path.filename().u8string() << " (" << stbi_failure_reason()
                  << ")\n";
    }
    glBindTexture(GL_TEXTURE_2D, texture_.id());
    glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data ? data : generated.data());
    if (data)
        stbi_image_free(data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, kind == "Ring" ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (GLEW_EXT_texture_filter_anisotropic) {
        float maximum = 1;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximum);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(8.0f, maximum));
    }
}
void Texture::bind(unsigned slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture_.id());
}
} // namespace sse
