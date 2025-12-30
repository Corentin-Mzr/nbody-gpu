#pragma once

#include <vector>
#include <glm/glm.hpp>

/*
- Mass in solar mass (1.9884e30 kg)
- Distance in light year (9.461e15 m)
- Time in 1M year (1M * 31 557 600s)
=> G = G_SI * MASS * TIME^2 / DISTANCE^3 = 1.56e-5
*/

struct Scene
{
    static constexpr std::size_t COUNT = 32768;
    static constexpr float DT = 1.0f / 60.0f;
    static constexpr float GRAVITY = 156000.f; // 1.0f;
    static constexpr std::size_t ITER_PER_FRAME = 1;
    static constexpr float SOFTENING = 156.0f;

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
[[nodiscard]]
Scene create_galaxy_collision_scene(uint32_t seed);

// Spherical generation with bias
[[nodiscard]]
Scene create_spheric_inequal(uint32_t seed);

// Big bang effect
[[nodiscard]]
Scene create_universe(uint32_t seed);

// Collapse effect
[[nodiscard]]
Scene create_sun_collapse(uint32_t seed);