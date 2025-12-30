#include "scene.hpp"
#include "constants.hpp"
#include <random>
#include <array>

[[nodiscard]]
static float hash(uint32_t seed)
{
    uint32_t state = seed * 747796405u + 2891336453u;
    uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    word = (word >> 22u) ^ word;

    return static_cast<float>(word) / static_cast<float>(UINT32_MAX);
}

[[nodiscard]]
static glm::vec4 star_color(uint32_t seed)
{
    float temperature = hash(seed) * 30000.0f + 3000.0f;

    glm::vec4 color(1.0f);

    // Red - Yellow
    if (temperature < 6600.0)
    {

        color.r = 1.0f;
        color.g = glm::mix(0.4f, 1.0f, (temperature - 3000.0f) / 3600.0f);
        color.b = glm::mix(0.2f, 1.0f, glm::smoothstep(5000.0f, 6600.0f, temperature));
    }

    // White - Blue
    else
    {
        color.r = glm::mix(1.0f, 0.7f, (temperature - 6600.0f) / 23400.0f);
        color.g = glm::mix(1.0f, 0.8f, (temperature - 6600.0f) / 23400.0f);
        color.b = 1.0f;
    }

    return color;
}

[[nodiscard]]
static glm::vec4 star_color(const glm::vec4 &pos)
{
    if (pos.x < 0)
    {
        return glm::vec4(0.8f, 0.5f, 0.3f, 1.0f);
    }
    return glm::vec4(0.2f, 0.6f, 0.3f, 1.0f);
}

Scene create_galaxy_bh_scene(uint32_t seed)
{
    Scene scene;

    std::mt19937 rng(seed);

    std::uniform_real_distribution<float> masses(MASS_MIN, MASS_MAX);
    std::uniform_real_distribution<float> angle(ANGLE_MIN, ANGLE_MAX);
    std::uniform_real_distribution<float> radius(RADIUS_MIN, RADIUS_MAX);

    scene.positions_and_masses[0] = glm::vec4(0.0f, 0.0f, 0.0f, BLACK_HOLE_MASS);
    scene.velocities[0] = glm::vec4(0.0f);

    for (std::size_t i = 1; i < Scene::COUNT; ++i)
    {
        float m = masses(rng);

        float r = radius(rng);
        float theta = angle(rng);

        float x = r * cos(theta);
        float y = GALAXY_THICKNESS * radius(rng) * ((rng() % 2) ? 1.0f : -1.0f);
        float z = r * sin(theta);
        scene.positions_and_masses[i] = glm::vec4(x, y, z, m);

        float v = sqrt(Scene::GRAVITY * BLACK_HOLE_MASS / r);
        float vx = -v * sin(theta);
        float vy = 0.0f;
        float vz = v * cos(theta);
        scene.velocities[i] = glm::vec4(vx, vy, vz, 0.0f);

        // scene.colors[i] = star_color(i * (seed + 1));
        scene.colors[i] = star_color({x, y, z, m});
    }

    return scene;
}

Scene create_galaxy_scene(uint32_t seed)
{
    Scene scene;

    std::mt19937 rng(seed);

    std::uniform_real_distribution<float> masses(MASS_MIN, MASS_MAX);
    std::uniform_real_distribution<float> angle(ANGLE_MIN, ANGLE_MAX);
    std::uniform_real_distribution<float> radius(RADIUS_MIN, 0.25f * RADIUS_MAX);

    for (std::size_t i = 0; i < Scene::COUNT; ++i)
    {
        float m = masses(rng); //* 3.8e5;

        float r = radius(rng);
        float theta = angle(rng);

        float x = r * cos(theta);
        float y = GALAXY_THICKNESS * radius(rng) * ((rng() % 2) ? 1.0f : -1.0f);
        float z = r * sin(theta);
        scene.positions_and_masses[i] = glm::vec4(x, y, z, m);

        float v = sqrt(Scene::GRAVITY * 100);
        float vx = -v * sin(theta);
        float vy = 0.0f;
        float vz = v * cos(theta);
        scene.velocities[i] = glm::vec4(vx, vy, vz, 0.0f);

        // scene.colors[i] = star_color(i * (seed + 1));
        scene.colors[i] = star_color({x, y, z, m});
    }

    return scene;
}

Scene create_galaxy_collision_scene(uint32_t seed)
{
    Scene scene;

    std::mt19937 rng(seed);

    std::uniform_real_distribution<float> masses(MASS_MIN, MASS_MAX);
    std::uniform_real_distribution<float> angle(ANGLE_MIN, ANGLE_MAX);
    std::uniform_real_distribution<float> radius(RADIUS_MIN, RADIUS_MAX);

    float x_offset = 1.25f * RADIUS_MAX;
    float vx_offset = -2000.0f;
    float vz_offset = -500.0f;

    // First galaxy
    scene.positions_and_masses[0] = glm::vec4(x_offset, 0.0f, 0.0f, BLACK_HOLE_MASS);
    scene.velocities[0] = glm::vec4(vx_offset, 0.0f, vz_offset, 0.0f);

    for (std::size_t i = 1; i < Scene::COUNT / 2; ++i)
    {
        float m = masses(rng);

        float r = radius(rng);
        float theta = angle(rng);

        float x = r * cos(theta) + x_offset;
        float y = GALAXY_THICKNESS * radius(rng) * ((rng() % 2) ? 1.0f : -1.0f);
        float z = r * sin(theta);
        scene.positions_and_masses[i] = glm::vec4(x, y, z, m);

        float v = sqrt(Scene::GRAVITY * BLACK_HOLE_MASS / r);
        float vx = -v * sin(theta) + vx_offset;
        float vy = 0.0f;
        float vz = v * cos(theta) + vz_offset;
        scene.velocities[i] = glm::vec4(vx, vy, vz, 0.0f);

        scene.colors[i] = glm::vec4(0.8f, 0.4f, 0.3f, 1.0f);
    }

    // Second galaxy
    scene.positions_and_masses[Scene::COUNT / 2] = glm::vec4(-x_offset, 0.0f, 0.0f, BLACK_HOLE_MASS);
    scene.velocities[Scene::COUNT / 2] = glm::vec4(-vx_offset, 0.0f, -vz_offset, 0.0f);

    for (std::size_t i = Scene::COUNT / 2 + 1; i < Scene::COUNT; ++i)
    {
        float m = masses(rng);

        float r = radius(rng);
        float theta = angle(rng);

        float x = r * cos(theta) - x_offset;
        float z = GALAXY_THICKNESS * radius(rng) * ((rng() % 2) ? 1.0f : -1.0f);
        float y = r * sin(theta);
        scene.positions_and_masses[i] = glm::vec4(x, y, z, m);

        float v = sqrt(Scene::GRAVITY * BLACK_HOLE_MASS / r);
        float vx = -v * sin(theta) - vx_offset;
        float vz = 0.0f;
        float vy = v * cos(theta) - vz_offset;
        scene.velocities[i] = glm::vec4(vx, vy, vz, 0.0f);

        scene.colors[i] = glm::vec4(0.3f, 0.7f, 0.2f, 1.0f);
    }

    return scene;
}

Scene create_spheric_inequal(uint32_t seed)
{
    Scene scene;

    std::mt19937 rng(seed);

    std::uniform_real_distribution<float> masses(MASS_MIN, MASS_MIN);
    std::uniform_real_distribution<float> longitude(ANGLE_MIN, ANGLE_MAX);
    std::uniform_real_distribution<float> colatitude(ANGLE_MIN, PI);
    std::uniform_real_distribution<float> radius(RADIUS_MIN, RADIUS_MIN);

    for (std::size_t i = 0; i < Scene::COUNT; ++i)
    {
        float m = 1.0f;
        float theta = longitude(rng);
        float phi = colatitude(rng);
        float r = radius(rng);

        float y = r * sin(phi) * cos(theta);
        float z = r * sin(phi) * sin(theta);
        float x = r * cos(phi);

        float vx = x / r;
        float vy = y / r;
        float vz = z / r;

        float cr = x >= 0.0f ? 1.0f : 0.0f;
        float cg = x < 0.0f ? 1.0f : 0.0f;
        float cb = 0.0f;

        scene.positions_and_masses[i] = glm::vec4(x, y, z, m);
        scene.velocities[i] = glm::vec4(vx, vy, vz, 0.0f);
        scene.colors[i] = glm::vec4(cr, cg, cb, 1.0f);
    }

    return scene;
}

Scene create_universe(uint32_t seed)
{
    Scene scene;

    std::mt19937 rng(seed);

    std::uniform_real_distribution<float> masses(MASS_MIN, MASS_MAX);
    std::uniform_real_distribution<float> longitude(ANGLE_MIN, ANGLE_MAX);
    std::uniform_real_distribution<float> radius(RADIUS_MIN, RADIUS_MIN);

    std::uniform_real_distribution<float> normal(0.0f, 1.0f);

    for (std::size_t i = 0; i < Scene::COUNT; ++i)
    {
        float m = masses(rng);
        float r = radius(rng);
        float theta = longitude(rng);

        float u = normal(rng);
        float phi = acosf(1 - 2 * u);

        float x = r * sin(phi) * cos(theta);
        float y = r * sin(phi) * sin(theta);
        float z = r * cos(phi);

        float vx = x / r;
        float vy = y / r;
        float vz = z / r;

        float cr = x >= 0.0f ? 0.4f : 0.6f;
        float cg = x < 0.0f ? 0.3f : 0.7f;
        float cb = 0.7f;

        scene.positions_and_masses[i] = glm::vec4(x, y, z, m);
        scene.velocities[i] = glm::vec4(vx, vy, vz, 0.0f);
        scene.colors[i] = glm::vec4(cr, cg, cb, 1.0f);
    }

    return scene;
}

Scene create_sun_collapse(uint32_t seed)
{
    Scene scene;

    std::mt19937 rng(seed);

    // std::uniform_real_distribution<float> masses(MASS_MIN, MASS_MAX);
    std::uniform_real_distribution<float> longitude(ANGLE_MIN, ANGLE_MAX);
    // std::uniform_real_distribution<float> radius(RADIUS_MAX, RADIUS_MAX);
    std::uniform_real_distribution<float> normal(0.0f, 1.0f);
    std::uniform_int_distribution<int> index(0, 4);

    static constexpr std::array<glm::vec4, 5> sun_colors = {
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(1.0f, 0.894f, 0.518f, 1.0f),
        glm::vec4(1.0f, 0.8f, 0.2f, 1.0f),
        glm::vec4(0.988f, 0.588f, 0.004f, 1.0f),
        glm::vec4(0.82f, 0.251f, 0.035f, 1.0f)
    };

    for (std::size_t i = 0; i < Scene::COUNT; ++i)
    {
        float m = MASS_MAX; //masses(rng);
        float r = RADIUS_MAX; //radius(rng);
        float theta = longitude(rng);

        float u = normal(rng);
        float phi = acosf(1 - 2 * u);

        float x = r * sin(phi) * cos(theta);
        float y = r * sin(phi) * sin(theta);
        float z = r * cos(phi);

        float vx = x / r;
        float vy = y / r;
        float vz = z / r;

        scene.positions_and_masses[i] = glm::vec4(x, y, z, m);
        scene.velocities[i] = glm::vec4(vx, vy, vz, 0.0f);
        scene.colors[i] = sun_colors[index(rng)];
    }

    return scene;
}