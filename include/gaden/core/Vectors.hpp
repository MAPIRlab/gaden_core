#pragma once
#define GLM_FORCE_INLINE
#define GLM_FORCE_XYZW_ONLY
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace gaden
{
    using Vector2 = glm::vec2;
    using Vector3 = glm::vec3;
    using Vector2i = glm::ivec2;
    using Vector3i = glm::ivec3;

    namespace vmath
    {
        template <typename Vec>
        inline float length(const Vec& vec)
        {
            return glm::length(vec);
        }

        template <typename Vec>
        inline float sqrlength(const Vec& vec)
        {
            return dot(vec, vec);
        }

        template <typename Vec>
        inline Vec normalized(const Vec& vec)
        {
            return glm::normalize(vec);
        }

        template <typename Vec>
        inline Vec cross(const Vec& a, const Vec& b)
        {
            return glm::cross(a, b);
        }

        template <typename Vec>
        inline float dot(const Vec& a, const Vec& b)
        {
            return glm::dot(a, b);
        }

        template <typename Vec>
        inline Vec ceil(const Vec& a)
        {
            return glm::ceil(a);
        }

        template <typename Vec>
        inline Vec lerp(const Vec& a, const Vec& b, float t)
        {
            Vec v;
            for (size_t i = 0; i < v.length(); i++)
                v[i] = std::lerp(a[i], b[i], t);

            return v;
        }

        template <typename Vec>
        inline Vec rotate(const Vec& vec, float signedAngleRadians)
        {
            return glm::rotate(vec, signedAngleRadians);
        }

        inline Vector2 transpose(const Vector2& vec)
        {
            return {vec.y, vec.x};
        }

        inline Vector3 WithZ(const Vector2& vec, float z)
        {
            return Vector3(vec.x, vec.y, z);
        }
    } // namespace vmath

} // namespace gaden

#include <fmt/format.h>
template <> struct fmt::formatter<gaden::Vector2i> : formatter<std::string>
{
    auto format(gaden::Vector2i const& v, format_context& ctx)
    {
        return fmt::format_to(ctx.out(), "({},{})", v.x, v.y);
    }
};

template <> struct fmt::formatter<gaden::Vector3i> : formatter<std::string>
{
    auto format(gaden::Vector3i const& v, format_context& ctx)
    {
        return fmt::format_to(ctx.out(), "({},{},{})", v.x, v.y, v.z);
    }
};

template <> struct fmt::formatter<gaden::Vector2> : formatter<std::string>
{
    auto format(gaden::Vector2 const& v, format_context& ctx)
    {
        return fmt::format_to(ctx.out(), "({:.2f},{:.2f})", v.x, v.y);
    }
};

template <> struct fmt::formatter<gaden::Vector3> : formatter<std::string>
{
    auto format(gaden::Vector3 const& v, format_context& ctx)
    {
        return fmt::format_to(ctx.out(), "({:.2f},{:.2f},{:.2f})", v.x, v.y, v.z);
    }
};
