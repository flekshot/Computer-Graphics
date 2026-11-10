#pragma once
#include "GLObject.h"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

namespace sse {
// A bounded, fully decoded GIF with one texture reused for every displayed frame.
class AnimatedTexture {
  public:
    explicit AnimatedTexture(const std::filesystem::path& path, bool monochrome = false);
    GLuint id() const { return ready_ ? texture_.id() : 0; }
    int width() const { return width_; }
    int height() const { return height_; }
    int frameCount() const { return static_cast<int>(frameEnds_.size()); }
    bool ready() const { return ready_; }
    void update(double elapsedSeconds, bool motion);

  private:
    struct ImageDeleter {
        void operator()(unsigned char* data) const noexcept;
    };
    GLObject texture_{GLKind::Texture};
    std::unique_ptr<unsigned char, ImageDeleter> pixels_;
    std::vector<double> frameEnds_;
    std::size_t frameBytes_ = 0;
    std::size_t currentFrame_ = 0;
    int width_ = 0;
    int height_ = 0;
    bool ready_ = false;
};
} // namespace sse
