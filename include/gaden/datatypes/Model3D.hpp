#pragma once
#include <filesystem>

namespace gaden
{
    struct Color
    {
        float r = 1, g = 1, b = 1, a = 1;

        static Color Parse(const std::string& str)
        {
            Color color;
            color.a = 1.0;

            std::stringstream ss(str);
            ss >> std::skipws;

            ss.ignore(256, '[');
            ss >> color.r;
            ss.ignore(256, ',');
            ss >> color.g;
            ss.ignore(256, ',');
            ss >> color.b;

            ss.ignore(256, ',');
            if (!ss.eof())
                ss >> color.a;

            return color;
        }

        bool operator==(Color const& other) const
        {
            return r == other.r &&
                   g == other.g &&
                   b == other.b &&
                   a == other.a;
        }

        bool operator!=(Color const& other) const
        {
            return !(*this == other);
        }
    };

    struct Model3D
    {
        std::filesystem::path path;
        Color color;

        operator std::filesystem::path() const
        {
            return path;
        }
    };
} // namespace gaden