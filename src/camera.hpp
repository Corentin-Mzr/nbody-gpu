#pragma once

#include <glm/glm.hpp>
#include "constants.hpp"

struct Camera
{
    static constexpr glm::vec3 UP{0.0f, 1.0f, 0.0f};
    static constexpr glm::vec3 CENTER{0.0f, 0.0f, 0.0f};
    static constexpr float FOV = 80.0f;          // in degrees
    static constexpr float ZOOM_FACTOR = 512.0f; // 256

    float r = 1.25f * RADIUS_MAX; // RADIUS_MAX
    float theta = 0.0f;     // in radians
    float phi = 0.0f;       // in radians

    Camera() = default;
    explicit Camera(float r, float theta = 0.0f, float phi = 0.0f)
        : r(r), theta(theta), phi(phi)
    {
    }
};

// Model View Projection matrix
[[nodiscard]]
glm::mat4 mvp_matrix(const Camera &cam, float width, float height);
