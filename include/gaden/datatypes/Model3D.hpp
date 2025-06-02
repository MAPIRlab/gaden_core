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
    };

    struct Model3D
    {
        std::filesystem::path path;
        Color color;
    };
} // namespace gaden