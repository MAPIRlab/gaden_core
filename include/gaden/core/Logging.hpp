#pragma once

#include "signal.h"
#include <fmt/color.h>
#include <fmt/format.h>

#if GADEN_ROS
#include <rclcpp/logging.hpp>
#endif

namespace gaden
{

    // clang-format off
    #if GADEN_ROS
    #ifndef GADEN_LOGGER_ID
    #define GADEN_LOGGER_ID "GADEN" // default, should be redefined per node
    #endif
    
    #define GADEN_INFO(...)                 RCLCPP_INFO(rclcpp::get_logger(GADEN_LOGGER_ID), "%s", fmt::format(__VA_ARGS__).c_str())
    #define GADEN_INFO_COLOR(color, ...)    RCLCPP_INFO(rclcpp::get_logger(GADEN_LOGGER_ID), "%s", fmt::format(fmt::fg(color), __VA_ARGS__).c_str())
    #define GADEN_WARN(...)                 RCLCPP_WARN(rclcpp::get_logger(GADEN_LOGGER_ID), "%s", fmt::format(__VA_ARGS__).c_str())
    #define GADEN_ERROR(...)                RCLCPP_ERROR(rclcpp::get_logger(GADEN_LOGGER_ID), "%s", fmt::format(__VA_ARGS__).c_str())
    
    #else
    
    #define GADEN_INFO(...)                 fprintf(stderr, "[INFO] %s\n", fmt::format(__VA_ARGS__).c_str())
    #define GADEN_INFO_COLOR(color, ...)    fprintf(stderr, "[INFO] %s\n", fmt::format(fmt::fg(color), __VA_ARGS__).c_str())
    #define GADEN_WARN(...)                 fprintf(stderr, "[WARN] %s\n", fmt::format(fmt::fg(fmt::terminal_color::yellow), __VA_ARGS__).c_str())
    #define GADEN_ERROR(...)                fprintf(stderr, "[ERROR] %s\n", fmt::format(fmt::fg(fmt::terminal_color::red), __VA_ARGS__).c_str())
    
    #endif

    // just a bit more visual emphasis for things that you really shouldn't ignore
    #define GADEN_SERIOUS_WARN(...)         GADEN_WARN( "{}", fmt::format(fmt::bg(fmt::terminal_color::yellow) | fmt::fg(fmt::terminal_color::black) | fmt::emphasis::bold, __VA_ARGS__).c_str())
    #define GADEN_SERIOUS_ERROR(...)        GADEN_ERROR( "{}", fmt::format(fmt::bg(fmt::terminal_color::red)   | fmt::fg(fmt::terminal_color::white) | fmt::emphasis::bold, __VA_ARGS__).c_str())

    // clang-format on

} // namespace gaden

#include <filesystem>
template <> struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string>
{
    auto format(std::filesystem::path const& v, format_context& ctx)
    {
        return fmt::format_to(ctx.out(), "{}", v.c_str());
    }
};
