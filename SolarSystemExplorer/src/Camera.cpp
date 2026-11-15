#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
namespace sse {
void Camera::focus(int index, const glm::vec3& anchor, float radius, bool rings) {
    followed_ = index;
    offset_ = center_ - anchor;
    pan_ = glm::vec3(0);
    minDistance_ = radius * (rings ? 3.0f : 1.42f);
    targetDistance_ = radius * (rings ? 8.5f : 6.0f);
    targetPitch_ = rings ? 0.52f : 0.30f;
    // The observer starts on the illuminated side; subsequent orbiting stays user-controlled.
    targetYaw_ = std::atan2(-anchor.x, -anchor.z) + 0.52f;
    targetYaw_ = yaw_ + std::remainder(targetYaw_ - yaw_, glm::two_pi<float>());
}
void Camera::setFocusRadius(float radius, bool rings) {
    const float minimum = radius * (rings ? 3.0f : 1.42f);
    if (followed_ < 0 || !std::isfinite(minimum) || minimum <= 0 || minimum > maxDistance_)
        return;
    const float scale = minimum / minDistance_;
    minDistance_ = minimum;
    targetDistance_ = std::clamp(targetDistance_ * scale, minDistance_, maxDistance_);
    distance_ = std::clamp(distance_ * scale, minDistance_, maxDistance_);
}
void Camera::overview() {
    followed_ = -1;
    offset_ = center_;
    pan_ = glm::vec3(0);
    minDistance_ = 4;
    targetDistance_ = overviewDistance_;
    targetYaw_ = yaw_ + std::remainder(0.62f - yaw_, glm::two_pi<float>());
    targetPitch_ = 0.64f;
}
void Camera::update(float dt, const glm::vec3& followedPosition) {
    if (!std::isfinite(dt) || dt <= 0)
        return;
    const float t = 1.0f - std::exp(-5.5f * dt);
    offset_ *= 1 - t;
    center_ = (followed_ >= 0 ? followedPosition : glm::vec3(0)) + offset_ + pan_;
    yaw_ += (targetYaw_ - yaw_) * t;
    pitch_ += (targetPitch_ - pitch_) * t;
    distance_ = std::max(minDistance_, std::exp(glm::mix(std::log(distance_), std::log(targetDistance_), t)));
    updateProjectionBlend(dt);
}
void Camera::updateProjectionBlend(float dt) {
    const float step = 1.0f - std::exp(-7.0f * dt);
    orthographicBlend_ += (orthographicTarget_ - orthographicBlend_) * step;
    if (std::abs(orthographicTarget_ - orthographicBlend_) < 1e-3f)
        orthographicBlend_ = orthographicTarget_;
}
void Camera::orbit(float dx, float dy) {
    targetYaw_ -= dx * 0.005f;
    targetPitch_ = std::clamp(targetPitch_ + dy * 0.005f, -1.48f, 1.48f);
}
void Camera::zoom(float wheel) {
    targetDistance_ = std::clamp(targetDistance_ * std::exp(-std::clamp(wheel, -40.0f, 40.0f) * 0.13f),
                                 minDistance_, maxDistance_);
}
void Camera::pan(float right, float forward, float dt) {
    const glm::vec3 r(std::cos(yaw_), 0, -std::sin(yaw_));
    const glm::vec3 f(-std::sin(yaw_), 0, -std::cos(yaw_));
    pan_ += (r * right + f * forward) * distance_ * 0.42f * dt;
}
glm::vec3 Camera::position() const {
    return center_ + distance_ * glm::vec3(std::cos(pitch_) * std::sin(yaw_), std::sin(pitch_),
                                           std::cos(pitch_) * std::cos(yaw_));
}
void Camera::avoidSphere(const glm::vec3& c, float radius) {
    auto d = position() - c;
    const float len = glm::length(d), safe = radius * 1.15f + 0.05f;
    if (len < safe) {
        auto shift = (len > 1e-5f ? d / len : glm::vec3(0, 1, 0)) * (safe - len);
        center_ += shift;
        // Escape a body without permanently changing the user's tracking center.
        offset_ += shift;
    }
}
glm::mat4 Camera::view() const {
    return glm::lookAt(position(), center_, glm::vec3(0, 1, 0));
}
void Camera::setViewport(float width, float height, bool panels, float sceneRadius) {
    if (width <= 0 || height <= 0)
        return;
    const float left = panels ? (width < 1200 ? 202.0f : 222.0f) : 0;
    const float right = panels ? (width < 1200 ? 248.0f : 284.0f) : 0;
    const float usable = std::max(240.0f, width - left - right - 64.0f);
    sceneRadius = std::isfinite(sceneRadius) ? std::clamp(sceneRadius, 60.0f, 400.0f) : 60.0f;
    maxDistance_ = std::max(360.0f, sceneRadius * 6.0f);
    const float overview = std::max(145.0f, sceneRadius * 2.414214f * height / (usable * 0.78f));
    if (followed_ < 0)
        targetDistance_ = std::clamp(targetDistance_ * overview / overviewDistance_, minDistance_, maxDistance_);
    overviewDistance_ = overview;
    projectionShift_ = (right - left) / width;
}
glm::mat4 Camera::perspectiveProjection(float aspect) const {
    auto p = glm::perspective(glm::radians(fieldOfViewDegrees), std::max(aspect, 0.01f), 0.025f,
                              std::max(1800.0f, maxDistance_ * 2.5f));
    // Off-centre frustum: shift the image so the scene is centred between the side panels.
    p[2][0] = projectionShift_;
    return p;
}
glm::mat4 Camera::orthographicProjection(float aspect) const {
    const float halfHeight = distance_ * std::tan(glm::radians(fieldOfViewDegrees) * 0.5f);
    const float halfWidth = halfHeight * std::max(aspect, 0.01f);
    auto p = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.025f,
                        std::max(1800.0f, maxDistance_ * 2.5f));
    // In a parallel projection w == 1, so the same panel offset is a translation in x.
    p[3][0] = -projectionShift_;
    return p;
}
glm::mat4 Camera::projection(float aspect) const {
    if (orthographicBlend_ <= 0.0f)
        return perspectiveProjection(aspect);
    if (orthographicBlend_ >= 1.0f)
        return orthographicProjection(aspect);
    // Both matrices agree on the focus plane, so a linear blend gives a smooth
    // "dolly" between the two projections without the subject jumping in size.
    const auto perspective = perspectiveProjection(aspect), orthographic = orthographicProjection(aspect);
    glm::mat4 blended;
    for (int column = 0; column < 4; ++column)
        blended[column] = glm::mix(perspective[column], orthographic[column], orthographicBlend_);
    return blended;
}
} // namespace sse
