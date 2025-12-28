#pragma once

#include <cstddef>
#include <iostream>
#include <string_view>
#include <format>

enum class ErrorType
{
    ShaderModuleCreation,
    ShaderModuleCompilation,
    ShaderProgramLinking,
    GLADInitialization,
};

inline void log_error(ErrorType type, std::string_view what)
{
    std::string_view error;
    switch (type)
    {
        case ErrorType::ShaderModuleCreation:
            error = "[SHADER MODULE CREATION ERROR]\n";
            break;
        case ErrorType::ShaderModuleCompilation:
            error = "[SHADER MODULE COMPILATION ERROR]\n";
            break;
        case ErrorType::ShaderProgramLinking:
            error = "[SHADER PROGRAM LINKING ERROR]\n";
            break;
        case ErrorType::GLADInitialization:
            error = "[GLAD INITIALIZATION ERROR]\n";
    }
    
    std::cerr << std::format("{}{}\n", error, what);
}