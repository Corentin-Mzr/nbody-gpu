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
    static constexpr std::size_t COUNT = 32768;
    static constexpr float DT = 1.0f / 60.0f;
    static constexpr float GRAVITY = 1.0f;
    static constexpr float DISTANCE_THRESHOLD = 1e-8f;

    // std::vector<float> masses;
    std::vector<glm::vec4> positions_and_masses; // x, y, z, m
    std::vector<glm::vec4> velocities;

    Scene()
        : positions_and_masses(COUNT, glm::vec4(0, 0, 0, 1)), velocities(COUNT)
    {
    }
};

struct ComputeUniforms
{
    GLint count;
    GLint dt;
    GLint gravity;
    GLint distance_threshold;
    GLint iter_per_frame;
};

struct RenderUniforms
{
    GLint mvp;
};

// Scene creation params
static constexpr std::size_t iteration_per_frame = 1;
static constexpr float PI = 3.141592653589793;
static constexpr float mass_min = 0.1f;
static constexpr float mass_max = 100.0f;
static constexpr float angle_min = 0.0f;
static constexpr float angle_max = 2.0 * PI;
static constexpr float radius_min = 200.0f;
static constexpr float radius_max = 2000.0f;
static constexpr float black_hole_mass = 1e6;
static constexpr float galaxy_thickness = 0.1f;

// Shader related
static GLuint compute_program = 0;
static GLuint render_program = 0;
static const std::filesystem::path compute_shader_filepath = "../shaders/compute.glsl";
static const std::filesystem::path vertex_shader_filepath = "../shaders/vertex.glsl";
static const std::filesystem::path fragment_shader_filepath = "../shaders/fragment.glsl";

// Work group size
static constexpr GLuint workgroup_size = 128;
static constexpr GLuint num_groups_x = (Scene::COUNT + workgroup_size - 1) / workgroup_size;
static constexpr GLuint num_groups_y = 1;
static constexpr GLuint num_groups_z = 1;

// Inputs
static bool reloaded = true;
static bool middle_pressed = false;
static bool first_motion = true;

// Camera - looking at (0, 0, 0) from a sphere
static float fov = 80.0f;
static float r = radius_max;
static float theta = 0.0f;
static float phi = 0.0f;
static float prev_xpos = 0.0f;
static float prev_ypos = 0.0f;
static float zoom_factor = 16.0f;

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
        compute_program = reload_compute_shader_program(compute_program, compute_shader_filepath);
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
    r = std::clamp(r, 0.5f * radius_min, 2.0f * radius_max);
}

std::string vec3_to_string(const glm::vec3 &v)
{
    return "Vec3(x=" + std::to_string(v.x) + ", y=" + std::to_string(v.y) + ", z=" + std::to_string(v.z) + ")";
}

Scene create_scene(uint32_t seed)
{
    Scene scene;

    std::mt19937 rng(seed);

    std::uniform_real_distribution<float> masses(mass_min, mass_max);
    std::uniform_real_distribution<float> angle(angle_min, angle_max);
    std::uniform_real_distribution<float> radius(radius_min, radius_max);

    scene.positions_and_masses[0] = glm::vec4(0.0f, 0.0f, 0.0f, black_hole_mass);
    scene.velocities[0] = glm::vec4(0.0f);

    for (std::size_t i = 1; i < Scene::COUNT; ++i)
    {
        float m = masses(rng);

        float r = radius(rng);
        float theta = angle(rng);

        float x = r * cos(theta);
        float y = galaxy_thickness * radius(rng) * ((rng() % 2) ? 1.0f : -1.0f);
        float z = r * sin(theta);
        scene.positions_and_masses[i] = glm::vec4(x, y, z, m);

        float v = sqrt(Scene::GRAVITY * black_hole_mass / r);
        float vx = -v * sin(theta);
        float vy = 0.0f;
        float vz = v * cos(theta);
        scene.velocities[i] = glm::vec4(vx, vy, vz, 0.0f);
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

    glfwSwapInterval(1);

    compute_program = make_compute_shader_program(compute_shader_filepath);
    if (compute_program == GL_FALSE)
    {
        std::cerr << "Could not create compute shader program\n";
        return -1;
    }

    render_program = make_shader_program(vertex_shader_filepath, fragment_shader_filepath);
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
    compute_uniforms.distance_threshold = glGetUniformLocation(compute_program, "distance_threshold");
    compute_uniforms.iter_per_frame = glGetUniformLocation(compute_program, "iter_per_frame");

    assert(compute_uniforms.count != -1);
    assert(compute_uniforms.dt != -1);
    assert(compute_uniforms.gravity != -1);
    assert(compute_uniforms.distance_threshold != -1);
    assert(compute_uniforms.iter_per_frame != -1);

    // Uniforms for render shader
    glUseProgram(render_program);
    RenderUniforms render_uniforms;
    render_uniforms.mvp = glGetUniformLocation(render_program, "mvp");

    assert(render_uniforms.mvp != -1);

    // Input data for compute shader
    Scene scene = create_scene(42);
    double current_time = 0.0;
    double last_time = 0.0;
    double acc = 0.0;

    // Input buffers
    GLuint positions_and_masses_buffer = 0;
    GLuint velocities_buffer = 0;

    glGenBuffers(1, &positions_and_masses_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positions_and_masses_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::COUNT * sizeof(scene.positions_and_masses[0]), scene.positions_and_masses.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positions_and_masses_buffer);

    glGenBuffers(1, &velocities_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, velocities_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::COUNT * sizeof(scene.velocities[0]), scene.velocities.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velocities_buffer);

    // Output buffers
    GLuint positions_and_masses_out = 0;

    glGenBuffers(1, &positions_and_masses_out);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positions_and_masses_out);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::COUNT * sizeof(scene.positions_and_masses[0]), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, positions_and_masses_out);

    // Unbind
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Rendering
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);

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
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positions_and_masses_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velocities_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, positions_and_masses_out);

            // Launch compute shader
            glUseProgram(compute_program);
            glUniform1ui(compute_uniforms.count, Scene::COUNT);
            glUniform1f(compute_uniforms.dt, Scene::DT);
            glUniform1f(compute_uniforms.gravity, Scene::GRAVITY);
            glUniform1f(compute_uniforms.distance_threshold, Scene::DISTANCE_THRESHOLD);
            glUniform1ui(compute_uniforms.iter_per_frame, iteration_per_frame);

            glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            std::swap(positions_and_masses_buffer, positions_and_masses_out);

            acc -= Scene::DT;
        }

        // Rendering
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positions_and_masses_buffer);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(fov), static_cast<float>(width) / static_cast<float>(height), 0.1f, 4.0f * radius_max);
        glm::mat4 view = glm::lookAt(glm::vec3(r * cos(phi) * sin(theta), r * sin(phi), r * cos(phi) * cos(theta)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 mvp = proj * view;

        glUseProgram(render_program);
        glUniformMatrix4fv(render_uniforms.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, Scene::COUNT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &positions_and_masses_buffer);
    glDeleteBuffers(1, &velocities_buffer);
    glDeleteBuffers(1, &positions_and_masses_out);
    glDeleteProgram(compute_program);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Goodbye World\n";

    return 0;
}