#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <format>
#include <filesystem>
#include <vector>
#include <random>
#include <cmath>
#include "shader.hpp"

struct Scene
{
    static constexpr std::size_t COUNT = 41000; //32768;
    static constexpr float DT = 1.0f / 60.0f;
    static constexpr float GRAVITY = 156000.f; // 1.0f;
    static constexpr std::size_t ITER_PER_FRAME = 1;
    static constexpr float SOFTENING = 150.0f;

    std::vector<glm::vec4> positions_and_masses; // x, y, z, m
    std::vector<glm::vec4> velocities;           // vx, vy, vz, 0
    std::vector<glm::vec4> colors;               // r, g, b, a

    Scene()
        : positions_and_masses(COUNT, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)), velocities(COUNT, glm::vec4(0.0f)), colors(COUNT, glm::vec4(1.0f))
    {
    }
};

struct ComputeUniforms
{
    GLint count;
    GLint dt;
    GLint gravity;
    GLint iter_per_frame;
    GLint softening;
};

struct RenderUniforms
{
    GLint mvp;
};

/*
- Mass in solar mass (1.9884e30 kg)
- Distance in light year (9.461e15 m)
- Time in 1M year (1M * 31 557 600s)

=> G = 1.56e-5
*/

// Scene creation params
static constexpr float PI = 3.141592653589793;
static constexpr float MASS_MIN = 0.1f;
static constexpr float MASS_MAX = 100.0f;
static constexpr float ANGLE_MIN = 0.0f;
static constexpr float ANGLE_MAX = 2.0 * PI;
static constexpr float RADIUS_MIN = 2000.0f;
static constexpr float RADIUS_MAX = 50000.0f;
static constexpr float BLACK_HOLE_MASS = 4.297e6;
static constexpr float GALAXY_THICKNESS = 0.05f;

// Shader related
static GLuint compute_program = 0;
static GLuint render_program = 0;
static const std::filesystem::path COMPUTE_SHADER_FILEPATH = "../shaders/compute.glsl";
static const std::filesystem::path VERTEX_SHADER_FILEPATH = "../shaders/vertex.glsl";
static const std::filesystem::path FRAGMENT_SHADER_FILEPATH = "../shaders/fragment.glsl";

// Work group size
static constexpr GLuint WORKGROUP_SIZE = 128;
static constexpr GLuint NUM_GROUPS_X = (Scene::COUNT + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
static constexpr GLuint NUM_GROUPS_Y = 1;
static constexpr GLuint NUM_GROUPS_Z = 1;

// Inputs
static bool reloaded = true;
static bool middle_pressed = false;
static bool first_motion = true;

// Camera - looking at (0, 0, 0) from a sphere
static float fov = 80.0f;
static float r = RADIUS_MAX;
static float theta = 0.0f;
static float phi = 0.0f;
static float prev_xpos = 0.0f;
static float prev_ypos = 0.0f;
static float zoom_factor = 256.0f;

static void error_callback(int error, const char *description)
{
    std::string message = "GLFW Error (" + std::to_string(error) + "): " + description + "\n";
    std::cerr << message;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        compute_program = reload_compute_shader_program(compute_program, COMPUTE_SHADER_FILEPATH);
        reloaded = (compute_program != 0);
    }
}

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        middle_pressed = true;
    }
    else
    {
        middle_pressed = false;
    }
}

static void mouse_motion_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (first_motion)
    {
        first_motion = false;
        prev_xpos = xpos;
        prev_ypos = ypos;
    }

    if (middle_pressed)
    {
        float dx = xpos - prev_xpos;
        float dy = ypos - prev_ypos;

        theta -= dx * 0.01f;
        phi += dy * 0.01f;

        phi = std::clamp(phi, -0.5f * PI + 0.01f, 0.5f * PI - 0.01f);
    }

    prev_xpos = xpos;
    prev_ypos = ypos;
}

static void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    r -= yoffset * zoom_factor;
    r = std::clamp(r, 0.5f * RADIUS_MIN, 2.0f * RADIUS_MAX);
}

std::string vec3_to_string(const glm::vec3 &v)
{
    return "Vec3(x=" + std::to_string(v.x) + ", y=" + std::to_string(v.y) + ", z=" + std::to_string(v.z) + ")";
}

float hash(uint32_t seed)
{
    uint32_t state = seed * 747796405u + 2891336453u;
    uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    word = (word >> 22u) ^ word;
    
    return static_cast<float>(word) / static_cast<float>(UINT32_MAX);
}

glm::vec4 star_color(uint32_t seed)
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

glm::vec4 star_color(const glm::vec4 &pos)
{
    if (pos.x < 0)
    {
        return glm::vec4(0.8f, 0.5f, 0.3f, 1.0f);
    }
    return glm::vec4(0.2f, 0.6f, 0.3f, 1.0f);
}

// Galaxy with black hole
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

// Galaxy no black hole
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

int main()
{
    std::cout << "Hello World\n";

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
    {
        return -1;
    }
    std::cout << "GLFW Init OK\n";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int width = 1280;
    int height = 720;

    GLFWwindow *window = glfwCreateWindow(width, height, "Compute Shader", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    std::cout << "GLFW Window OK\n";

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_motion_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    glfwSwapInterval(1);

    compute_program = make_compute_shader_program(COMPUTE_SHADER_FILEPATH);
    if (compute_program == GL_FALSE)
    {
        std::cerr << "Could not create compute shader program\n";
        return -1;
    }

    render_program = make_shader_program(VERTEX_SHADER_FILEPATH, FRAGMENT_SHADER_FILEPATH);
    if (render_program == GL_FALSE)
    {
        std::cerr << "Could not create render shader program\n";
        return -1;
    }

    // Uniforms for compute shader
    glUseProgram(compute_program);
    ComputeUniforms compute_uniforms;
    compute_uniforms.count = glGetUniformLocation(compute_program, "count");
    compute_uniforms.dt = glGetUniformLocation(compute_program, "dt");
    compute_uniforms.gravity = glGetUniformLocation(compute_program, "gravity");
    compute_uniforms.iter_per_frame = glGetUniformLocation(compute_program, "iter_per_frame");
    compute_uniforms.softening = glGetUniformLocation(compute_program, "softening");

    assert(compute_uniforms.count != -1);
    assert(compute_uniforms.dt != -1);
    assert(compute_uniforms.gravity != -1);
    assert(compute_uniforms.iter_per_frame != -1);
    assert(compute_uniforms.softening != -1);

    // Uniforms for render shader
    glUseProgram(render_program);
    RenderUniforms render_uniforms;
    render_uniforms.mvp = glGetUniformLocation(render_program, "mvp");

    assert(render_uniforms.mvp != -1);

    // Input buffers
    GLuint positions_and_masses_in = 0;
    GLuint velocities_buffer = 0;
    GLuint colors_buffer = 0;

    // Output buffers
    GLuint positions_and_masses_out = 0;

    // Input data for compute shader
    Scene scene = create_galaxy_collision_scene(42);

    glGenBuffers(1, &positions_and_masses_in);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positions_and_masses_in);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::COUNT * sizeof(scene.positions_and_masses[0]), scene.positions_and_masses.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positions_and_masses_in);

    glGenBuffers(1, &velocities_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, velocities_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::COUNT * sizeof(scene.velocities[0]), scene.velocities.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velocities_buffer);

    glGenBuffers(1, &colors_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, colors_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::COUNT * sizeof(scene.colors[0]), scene.colors.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, colors_buffer);

    glGenBuffers(1, &positions_and_masses_out);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positions_and_masses_out);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::COUNT * sizeof(scene.positions_and_masses[0]), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, positions_and_masses_out);

    // Unbind
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Rendering
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindBuffer(GL_ARRAY_BUFFER, positions_and_masses_in);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)0);
    glBindVertexArray(0);

    // Simulation time
    double current_time = 0.0;
    double last_time = 0.0;
    double acc = 0.0;

    std::cout << "Main loop start\n";

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &width, &height);
        current_time = glfwGetTime();
        acc += current_time - last_time;
        last_time = current_time;

        if (acc >= 0.25)
        {
            acc = 0.25;
        }

        while (acc >= Scene::DT)
        {
            // Rebind buffers
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positions_and_masses_in);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velocities_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, colors_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, positions_and_masses_out);

            // Launch compute shader
            glUseProgram(compute_program);
            glUniform1ui(compute_uniforms.count, Scene::COUNT);
            glUniform1f(compute_uniforms.dt, Scene::DT);
            glUniform1f(compute_uniforms.gravity, Scene::GRAVITY);
            glUniform1ui(compute_uniforms.iter_per_frame, Scene::ITER_PER_FRAME);
            glUniform1f(compute_uniforms.softening, Scene::SOFTENING);

            glDispatchCompute(NUM_GROUPS_X, NUM_GROUPS_Y, NUM_GROUPS_Z);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

            std::swap(positions_and_masses_in, positions_and_masses_out);

            acc -= Scene::DT;
        }

        // Rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_FALSE);

        glm::mat4 proj = glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), 1.0f, 8.0f * RADIUS_MAX);
        glm::mat4 view = glm::lookAt(glm::vec3(r * cos(phi) * sin(theta), r * sin(phi), r * cos(phi) * cos(theta)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 mvp = proj * view;

        glUseProgram(render_program);
        glUniformMatrix4fv(render_uniforms.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, positions_and_masses_in);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, colors_buffer);
        glDrawArrays(GL_POINTS, 0, Scene::COUNT);

        // After render
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &positions_and_masses_in);
    glDeleteBuffers(1, &velocities_buffer);
    glDeleteBuffers(1, &colors_buffer);
    glDeleteBuffers(1, &positions_and_masses_out);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(compute_program);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Goodbye World\n";

    return 0;
}