#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <format>
#include <filesystem>
#include <cmath>
#include "shader.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "constants.hpp"
#include "error_log.hpp"

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

// Inputs
struct Input
{
    bool reloaded_shaders = true;
    bool middle_button_pressed = false;
    bool first_motion = true;
    bool pause_simulation = true;
    float xpos = 0.0f;
    float ypos = 0.0f;
};
static Input input;

// Camera - looking at (0, 0, 0) from a sphere
static Camera camera(1.25f * RADIUS_MAX, 0.0f, glm::radians(30.0f));

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

static constexpr std::string_view debug_source_to_string(GLenum source) noexcept
{
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "WINDOW_SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "SHADER_COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "APPLICATION";
    case GL_DEBUG_SOURCE_OTHER:
        return "OTHER";
    default:
        return "UNKNOWN_SOURCE";
    }
}

static constexpr std::string_view debug_type_to_string(GLenum type) noexcept
{
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "PERFORMANCE";
    case GL_DEBUG_TYPE_MARKER:
        return "MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "PUSH_GROUP";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "POP_GROUP";
    case GL_DEBUG_TYPE_OTHER:
        return "OTHER";
    default:
        return "UNKNOWN_TYPE";
    }
}

static constexpr std::string_view debug_severity_to_string(GLenum severity) noexcept
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        return "HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "MEDIUM";
    case GL_DEBUG_SEVERITY_LOW:
        return "LOW";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "NOTIFICATION";
    default:
        return "UNKNOWN_SEVERITY";
    }
}

static void GLAPIENTRY opengl_error_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar *message, const void * /*userParam*/)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return;
    }

    std::string_view msg{message};
    std::cerr << std::format("[OpenGL] Source={} | Type={} | Severity={} | ID={}\n{}\n",
                             debug_source_to_string(source),
                             debug_type_to_string(type),
                             debug_severity_to_string(severity), id, msg);
}

static void glfw_error_callback(int error, const char *description)
{
    std::string message = "GLFW Error (" + std::to_string(error) + "): " + description + "\n";
    std::cerr << message;
}

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        compute_program = reload_compute_shader_program(compute_program, COMPUTE_SHADER_FILEPATH);
        render_program = reload_shader_program(render_program, VERTEX_SHADER_FILEPATH, FRAGMENT_SHADER_FILEPATH);
        input.reloaded_shaders = (compute_program != 0) || (render_program != 0);
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        input.pause_simulation = !input.pause_simulation;
    }
}

static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        input.middle_button_pressed = true;
    }
    else
    {
        input.middle_button_pressed = false;
    }
}

static void glfw_mouse_motion_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (input.first_motion)
    {
        input.first_motion = false;
        input.xpos = xpos;
        input.ypos = ypos;
    }

    if (input.middle_button_pressed)
    {
        float dx = xpos - input.xpos;
        float dy = ypos - input.ypos;

        camera.theta -= dx * 0.01f;
        camera.phi += dy * 0.01f;

        camera.phi = std::clamp(camera.phi, -0.5f * PI + 0.01f, 0.5f * PI - 0.01f);
    }

    input.xpos = xpos;
    input.ypos = ypos;
}

static void glfw_mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.r -= yoffset * camera.ZOOM_FACTOR;
    camera.r = std::clamp(camera.r, 0.5f * RADIUS_MIN, 2.0f * RADIUS_MAX);
}

[[nodiscard]]
std::string vec3_to_string(const glm::vec3 &v)
{
    return "Vec3(x=" + std::to_string(v.x) + ", y=" + std::to_string(v.y) + ", z=" + std::to_string(v.z) + ")";
}

int main()
{
    std::cout << "Hello World\n";

    glfwSetErrorCallback(glfw_error_callback);

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

    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetCursorPosCallback(window, glfw_mouse_motion_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetScrollCallback(window, glfw_mouse_scroll_callback);
    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        log_error(ErrorType::GLADInitialization, "Failed to initialize GLAD");
        return -1;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    // Enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(opengl_error_callback, 0);

    glfwSwapInterval(1);

    compute_program = make_compute_shader_program(COMPUTE_SHADER_FILEPATH);
    if (compute_program == GL_FALSE)
    {
        return -1;
    }

    render_program = make_shader_program(VERTEX_SHADER_FILEPATH, FRAGMENT_SHADER_FILEPATH);
    if (render_program == GL_FALSE)
    {
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
    Scene scene = create_galaxy_bh_scene(42);

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
    glBindVertexArray(vao);
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

        while (acc >= Scene::DT && !input.pause_simulation)
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
        glm::mat4 mvp = mvp_matrix(camera, static_cast<float>(width), static_cast<float>(height));

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_FALSE);

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