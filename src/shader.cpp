#include "shader.hpp"
#include "error_log.hpp"
#include <fstream>
#include <sstream>
#include <vector>

static std::string read_file(const std::filesystem::path &filepath)
{
    std::ifstream file;
    std::stringstream buffer;
    std::string line;
    buffer.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try 
    {
        file.open(filepath);
        buffer << file.rdbuf();
        file.close();
    }
    catch (std::ifstream::failure &e) 
    {
        log_error(ErrorType::ShaderModuleCreation, e.what());
    }

    return buffer.str();
}

GLuint make_shader_module(const std::filesystem::path &filepath, GLenum module_type)
{
    std::string shader_source = read_file(filepath);
    const GLchar* shader_src = static_cast<const GLchar*>(shader_source.c_str());

    // Create and compile the shader module
    GLuint shader_module = glCreateShader(module_type);
    glShaderSource(shader_module, 1, &shader_src, NULL);
    glCompileShader(shader_module);

    // Check compilation is OK
    GLint is_compiled = 0;
    glGetShaderiv(shader_module, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == GL_FALSE)
    {
        GLint max_length = 0;
        glGetShaderiv(shader_module, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> info_log(max_length);
        glGetShaderInfoLog(shader_module, max_length, &max_length, &info_log[0]);

        log_error(ErrorType::ShaderModuleCompilation, info_log.data());
    }

    return shader_module;
}

GLuint make_shader_program(const std::filesystem::path &vertex_filepath, const std::filesystem::path &fragment_filepath)
{
    // Create modules
    GLuint vertex_shader = make_shader_module(vertex_filepath, GL_VERTEX_SHADER);
    GLuint fragment_shader = make_shader_module(fragment_filepath, GL_FRAGMENT_SHADER);

    // Create shader program and link modules
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // Check linkage is OK
    GLint is_linked = 0;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &is_linked);
    if (is_linked == GL_FALSE)
    {
        GLint max_length = 0;
        glGetShaderiv(shader_program, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> info_log(max_length);
        glGetShaderInfoLog(shader_program, max_length, &max_length, &info_log[0]);

        log_error(ErrorType::ShaderProgramLinking, info_log.data());

        glDeleteProgram(shader_program);
    }

    // Delete modules
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

GLuint make_compute_shader_program(const std::filesystem::path &compute_filepath)
{
    // Create module
    GLuint compute_shader = make_shader_module(compute_filepath, GL_COMPUTE_SHADER);

    // Create shader program and link module
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, compute_shader);
    glLinkProgram(shader_program);

    // Check linkage is OK
    GLint is_linked = 0;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &is_linked);
    if (is_linked == GL_FALSE)
    {
        GLint max_length = 0;
        glGetShaderiv(shader_program, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> info_log(max_length);
        glGetShaderInfoLog(shader_program, max_length, &max_length, &info_log[0]);

        log_error(ErrorType::ShaderProgramLinking, info_log.data());

        glDeleteProgram(shader_program);
    }

    // Delete module
    glDeleteShader(compute_shader);

    return shader_program;
}

GLuint reload_compute_shader_program(GLuint program, const std::filesystem::path &compute_filepath)
{
    GLuint new_program = make_compute_shader_program(compute_filepath);
    if (new_program == GL_FALSE)
    {
        return program;
    }

    glDeleteProgram(program);
    return new_program;
}