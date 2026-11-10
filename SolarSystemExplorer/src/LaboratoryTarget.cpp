#include "LaboratoryTarget.h"
#include <algorithm>
#include <stdexcept>

namespace sse {
namespace {
void checkTarget() {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Laboratory framebuffer is incomplete");
}
} // namespace

LaboratoryTarget::LaboratoryTarget(const std::filesystem::path& root)
    : composite_(root / "shaders/fullscreen.vert", root / "shaders/laboratory_composite.frag") {}

void LaboratoryTarget::begin(int width, int height) {
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Laboratory target requires a positive framebuffer size");
    if (width != width_ || height != height_) {
        GLint maximum = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &maximum);
        const int samples = std::max(1, std::min(4, maximum));
        glBindFramebuffer(GL_FRAMEBUFFER, multisampleFbo_.id());
        glBindRenderbuffer(GL_RENDERBUFFER, color_.id());
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA16F, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_.id());
        glBindRenderbuffer(GL_RENDERBUFFER, depth_.id());
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_.id());
        checkTarget();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resolvedColor_.id());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, resolvedFbo_.id());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolvedColor_.id(), 0);
        checkTarget();
        width_ = width;
        height_ = height;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, multisampleFbo_.id());
    glViewport(0, 0, width_, height_);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_FRAMEBUFFER_SRGB);
}

void LaboratoryTarget::present(float exposure, bool normalColors) {
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampleFbo_.id());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvedFbo_.id());
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    composite_.use();
    composite_.set("uScene", 0);
    composite_.set("uExposure", exposure);
    composite_.set("uNormalColors", int(normalColors));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, resolvedColor_.id());
    glBindVertexArray(fullscreenVao_.id());
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
} // namespace sse
