#pragma once

#include "signal.h"
#include <fmt/color.h>
#include <fmt/format.h>

#if USING_ROS
#include <rclcpp/logging.hpp>
#endif

namespace gaden
{

#if USING_ROS
#ifndef GADEN_LOGGER_ID
#define GADEN_LOGGER_ID "GADEN" // default, should be redefined per node
#endif

#define GADEN_INFO(...) RCLCPP_INFO(rclcpp::get_logger(GADEN_LOGGER_ID), "%s", fmt::format(__VA_ARGS__).c_str())
#define GADEN_INFO_COLOR(color, ...) RCLCPP_INFO(rclcpp::get_logger(GADEN_LOGGER_ID), "%s", fmt::format(fmt::fg(color), __VA_ARGS__).c_str())

#define GADEN_WARN(...) RCLCPP_WARN(rclcpp::get_logger(GADEN_LOGGER_ID), "%s", fmt::format(__VA_ARGS__).c_str())

#define GADEN_ERROR(...) RCLCPP_ERROR(rclcpp::get_logger(GADEN_LOGGER_ID), "%s", fmt::format(__VA_ARGS__).c_str())

#else

#define GADEN_INFO(...) printf("[INFO] %s", fmt::format(__VA_ARGS__).c_str())
#define GADEN_INFO_COLOR(color, ...) printf("[INFO] %s", fmt::format(fmt::fg(color), __VA_ARGS__).c_str())

#define GADEN_WARN(...) printf("[WARN] %s", fmt::format(fmt::fg(fmt::terminal_color::yellow), __VA_ARGS__).c_str())

#define GADEN_ERROR(...) printf("[ERROR] %s", fmt::format(fmt::fg(fmt::terminal_color::red), __VA_ARGS__).c_str())

#endif

#if GADEN_DEBUG
#define GADEN_ASSERT_MSG(cnd, msg)                                                                                                                    \
    {                                                                                                                                                 \
        if (!(cnd))                                                                                                                                   \
        {                                                                                                                                             \
            GADEN_ERROR("{0}:     At {1}",                                                                                                            \
                        fmt::format(fmt::bg(fmt::terminal_color::red) | fmt::fg(fmt::terminal_color::white) | fmt::emphasis::bold, "ERROR: {}", msg), \
                        fmt::format(fmt::emphasis::bold, "{0}:{1}", __FILE__, __LINE__));                                                             \
            raise(SIGTRAP);                                                                                                                           \
        }                                                                                                                                             \
    }

#else

#define GADEN_ASSERT_MSG(cnd, msg)

#endif

#define GADEN_ASSERT(cnd) GADEN_ASSERT_MSG(cnd, "")
#define GADEN_TERMINATE raise(SIGTRAP)
} // namespace gaden

#include <filesystem>
template <> struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string>
{
    auto format(std::filesystem::path const& v, format_context& ctx)
    {
        return fmt::format_to(ctx.out(), "{}", v.c_str());
    }
};
