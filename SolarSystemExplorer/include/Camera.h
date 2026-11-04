#pragma once
#include <glm/glm.hpp>
namespace sse {
class Camera {
  public:
    void focus(int index, const glm::vec3& anchor, float radius, bool rings = false);
    void overview();
    void update(float dt, const glm::vec3& followedPosition);
    void orbit(float deltaX, float deltaY);
    void zoom(float wheel);
    void pan(float right, float forward, float dt);
    void avoidSphere(const glm::vec3& center, float radius);
    void setViewport(float width, float height, bool panels);
    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
    glm::vec3 position() const;
    const glm::vec3& center() const { return center_; }
    int followed() const { return followed_; }
    float distance() const { return distance_; }

  private:
    glm::vec3 center_{0}, offset_{0}, pan_{0};
    float yaw_ = 0.62f, pitch_ = 0.64f, targetYaw_ = 0.62f, targetPitch_ = 0.64f;
    float distance_ = 158, targetDistance_ = 158, minDistance_ = 4;
    float overviewDistance_ = 158, projectionShift_ = 0;
    int followed_ = -1;
};
} // namespace sse
