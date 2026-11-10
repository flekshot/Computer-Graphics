#include "Camera.h"
#include "Geometry.h"
#include "SolarSystem.h"
#include "VisualMath.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
bool near(double a, double b, double eps = 1e-4) {
    return std::abs(a - b) < eps;
}
} // namespace
int main() {
    try {
        for (double boundary : {500.0, 1500.0, 9999999500.0}) {
            const auto before = glm::rotate(glm::mat4(1), sse::cloudRotationRadians(boundary - .01), glm::vec3(0, 1, 0));
            const auto after = glm::rotate(glm::mat4(1), sse::cloudRotationRadians(boundary + .01), glm::vec3(0, 1, 0));
            check(glm::length(glm::vec3((before - after) * glm::vec4(1, 0, 0, 0))) < .002f,
                  "Earth clouds must rotate continuously across their time wrap");
        }
        for (float aspect : {.15f, .34f, 1.0f, 2.0f}) {
            const float fov = sse::fitVerticalFov(1.17f, aspect);
            check(fov > 0 && fov < 3.14159265f,
                  "An eclipse observer projection must never invert in a portrait window");
            const auto projection = glm::perspective(fov, aspect, .01f, 100.0f);
            check(projection[0][0] > 0 && projection[1][1] > 0,
                  "Observer projection must keep both screen axes upright");
        }
        sse::SolarSystem system;
        check(system.planets().size() == 8, "All eight planets must participate in the model");
        for (const auto& p : system.planets()) {
            check(glm::length(p.position(0) - p.position(p.data().orbitalDays)) < 0.0002f,
                  "An orbit must close after one period");
            check(near(glm::length(p.position(817)), p.data().orbitRadius),
                  "Orbit distance must remain stable at arbitrary times");
            check(glm::length(p.position(0) - p.position(p.data().orbitalDays / 2)) >
                      p.data().orbitRadius * 1.99f,
                  "Half period must reach opposite side");
        }
        system.update(1.0);
        check(near(system.days(), 6), "Default time conversion: 6 simulated days per second");
        system.setPaused(true);
        system.update(10);
        check(near(system.days(), 6), "Paused simulation must not advance");
        system.setPaused(false);
        system.setSpeed(2);
        system.update(0.5);
        check(near(system.days(), 12), "Speed must scale elapsed time");
        sse::SolarSystem partitioned;
        for (int i = 0; i < 120; ++i)
            partitioned.update(1.0 / 120.0);
        check(near(partitioned.days(), 6), "Frame partition must not change simulated time");
        system.update(-1);
        system.update(std::numeric_limits<double>::quiet_NaN());
        check(near(system.days(), 12), "Invalid delta times must not corrupt simulation");
        for (double t : {0.0, 27.3217, 3600.0, 1e8}) {
            const auto relative = system.moonPosition(t) - system.planets()[2].position(t);
            check(near(glm::length(relative), 2.3, 0.0002),
                  "Moon must remain attached to Earth's translated frame");
        }
        const auto sphere = sse::makeSphere(64, 32);
        for (const auto& v : sphere.vertices) {
            check(near(glm::length(v.normal), 1), "Sphere normals must be unit length");
            check(glm::dot(v.position, v.normal) > 0.999f, "Sphere normals must point outwards");
            check(v.uv.x >= 0 && v.uv.x <= 1 && v.uv.y >= 0 && v.uv.y <= 1,
                  "UVs must fit normalized texture coordinates");
        }
        for (size_t i = 0; i < sphere.indices.size(); i += 3) {
            auto a = sphere.vertices.at(sphere.indices[i]).position;
            auto b = sphere.vertices.at(sphere.indices[i + 1]).position;
            auto c = sphere.vertices.at(sphere.indices[i + 2]).position;
            auto normal = glm::cross(b - a, c - a);
            if (glm::length(normal) > 1e-7f)
                check(glm::dot(normal, a + b + c) > 0, "Triangles must use outward CCW winding");
        }
        bool rejected = false;
        try {
            (void)sse::makeSphere(2, 1);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Invalid tessellation must be rejected");
        sse::Camera camera;
        const glm::vec3 anchor(14, 1, -5);
        camera.focus(2, anchor, 1.0f);
        const auto before = camera.center();
        camera.update(1.0f / 60, anchor);
        check(glm::length(camera.center() - anchor) > 0.1f, "Focus must interpolate instead of teleporting");
        check(glm::length(camera.center() - anchor) < glm::length(before - anchor),
              "Focus must approach the selected target");
        for (int i = 0; i < 300; ++i)
            camera.update(1.0f / 60, anchor);
        check(glm::length(camera.center() - anchor) < 0.001f, "Focus must settle onto the target");
        const glm::vec3 next = anchor + glm::vec3(4, 0, 3);
        camera.update(1.0f / 60, next);
        check(glm::length(camera.center() - next) < 0.001f,
              "A settled camera must follow moving planets without position lag");
        camera.zoom(1000);
        for (int i = 0; i < 300; ++i)
            camera.update(1.0f / 60, next);
        check(glm::length(camera.position() - next) > 1.2f,
              "Zoom must never put the camera inside its focused planet");
        camera.orbit(10000, 10000);
        camera.update(1, next);
        const auto view = camera.view();
        for (int x = 0; x < 4; ++x)
            for (int y = 0; y < 4; ++y)
                check(std::isfinite(view[x][y]), "View matrix must remain finite at pitch limits");
        camera.overview();
        for (int i = 0; i < 300; ++i)
            camera.update(1.0f / 60, {});
        check(camera.followed() < 0 && glm::length(camera.center()) < 0.001f,
              "Overview must detach and return to origin");
        sse::Camera sculpting;
        sculpting.focus(0, {}, 1);
        sculpting.orbit(160, -90);
        sculpting.zoom(2);
        for (int i = 0; i < 300; ++i)
            sculpting.update(1.0f / 60, {});
        sculpting.pan(.2f, -.1f, .3f);
        sculpting.update(1.0f / 60, {});
        const auto sculptingCenter = sculpting.center();
        const auto sculptingDirection = glm::normalize(sculpting.position() - sculptingCenter);
        const float sculptingDistance = sculpting.distance();
        sculpting.setFocusRadius(1.5f);
        for (int i = 0; i < 300; ++i)
            sculpting.update(1.0f / 60, {});
        check(glm::length(sculpting.center() - sculptingCenter) < .0001f,
              "Changing terrain size must preserve user pan");
        check(glm::length(glm::normalize(sculpting.position() - sculpting.center()) - sculptingDirection) < .0001f,
              "Changing terrain size must preserve viewing direction");
        check(near(sculpting.distance(), sculptingDistance * 1.5f),
              "Changing terrain size must preserve relative zoom");
        sculpting.setFocusRadius(0);
        sculpting.setFocusRadius(std::numeric_limits<float>::quiet_NaN());
        check(near(sculpting.distance(), sculptingDistance * 1.5f),
              "Invalid terrain bounds must leave the camera intact");
        sculpting.zoom(1000);
        for (int i = 0; i < 300; ++i)
            sculpting.update(1.0f / 60, {});
        check(sculpting.distance() >= 1.5f * 1.42f,
              "Zoom limits must follow the resized terrain");
        // Collision corrections during a focus flight must not become permanent user pan.
        sse::Camera flight;
        auto settle = [&](int index) {
            const auto& destination = system.planets()[index];
            flight.focus(index, destination.position(0), destination.data().radius, index == 5);
            for (int i = 0; i < 600; ++i) {
                flight.update(1.0f / 60, destination.position(0));
                flight.avoidSphere({0, 0, 0}, sse::SolarSystem::sunRadius);
                for (const auto& body : system.planets())
                    flight.avoidSphere(body.position(0), body.data().radius);
                flight.avoidSphere(system.moonPosition(0), sse::SolarSystem::moonRadius);
            }
            check(glm::length(flight.center() - destination.position(0)) < 0.002f,
                  "A focus flight crossing another body must still settle onto its destination");
        };
        settle(3);
        settle(1);
        settle(6);
        settle(0);
        sse::Camera fastCamera;
        fastCamera.focus(0, system.planets()[0].position(0), system.planets()[0].data().radius);
        for (int i = 1; i <= 900; ++i) {
            const double day = i * 10.0; // 100x speed at 60 updates/second.
            fastCamera.update(1.0f / 60, system.planets()[0].position(day));
            fastCamera.avoidSphere({0, 0, 0}, sse::SolarSystem::sunRadius);
            for (const auto& body : system.planets())
                fastCamera.avoidSphere(body.position(day), body.data().radius);
        }
        check(glm::length(fastCamera.center() - system.planets()[0].position(9000)) < 0.01f,
              "Accelerated Mercury tracking must recover from transient collision corrections");
        for (const glm::vec2 dimensions : {glm::vec2(1440, 900), glm::vec2(1000, 640)}) {
            sse::Camera overview;
            overview.setViewport(dimensions.x, dimensions.y, true);
            for (int i = 0; i < 600; ++i)
                overview.update(1.0f / 60, {});
            const auto vp = overview.projection(dimensions.x / dimensions.y) * overview.view();
            const float leftEdge = dimensions.x < 1200 ? 218.0f : 238.0f;
            const float rightEdge = dimensions.x - (dimensions.x < 1200 ? 264.0f : 300.0f);
            for (const auto& body : system.planets()) {
                const auto clip = vp * glm::vec4(body.position(0), 1);
                const float x = (clip.x / clip.w * 0.5f + 0.5f) * dimensions.x;
                check(x > leftEdge && x < rightEdge,
                      "All eight destination centers must be visible between panels in the initial overview");
            }
        }
        std::cout << "PASS: " << checks << " core assertions (orbits, time, hierarchy, mesh, camera)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << " assertions: " << e.what() << '\n';
        return 1;
    }
}
