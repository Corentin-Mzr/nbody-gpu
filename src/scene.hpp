#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Scene
{
    static constexpr std::size_t COUNT = 32768;
    static constexpr float DT = 1.0f / 60.0f;
    static constexpr float GRAVITY = 156000.f; // 1.0f;
    static constexpr std::size_t ITER_PER_FRAME = 1;
    static constexpr float SOFTENING = 150.0f;

    std::vector<glm::vec4> positions_and_masses; // x, y, z, m
    std::vector<glm::vec4> velocities;           // vx, vy, vz, 0
    std::vector<glm::vec4> colors;               // r, g, b, a

    Scene()
        : positions_and_masses(COUNT, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)),
          velocities(COUNT, glm::vec4(0.0f)),
          colors(COUNT, glm::vec4(1.0f))
    {
    }
};


// Galaxy with black hole
[[nodiscard]]
Scene create_galaxy_bh_scene(uint32_t seed);

// Galaxy no black hole
[[nodiscard]]
Scene create_galaxy_scene(uint32_t seed);

// Two galaxies colliding
Scene create_galaxy_collision_scene(uint32_t seed);