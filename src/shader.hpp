#pragma once

#include <string_view>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <filesystem>

// Create a shader module from file
[[nodiscard]] 
GLuint make_shader_module(const std::filesystem::path &filepath, GLenum module_type);

// Create a shader program from a vertex and fragment shader files
[[nodiscard]] 
GLuint make_shader_program(const std::filesystem::path &vertex_filepath, const std::filesystem::path &fragment_filepath);

// Create a compute shader program from a compute shader file
[[nodiscard]]
GLuint make_compute_shader_program(const std::filesystem::path &compute_filepath);

[[nodiscard]]
GLuint reload_shader_program(GLuint program, const std::filesystem::path &vertex_filepath, const std::filesystem::path &fragment_filepath);

// Reload a compute shader program from file
[[nodiscard]]
GLuint reload_compute_shader_program(GLuint program, const std::filesystem::path &compute_filepath);
