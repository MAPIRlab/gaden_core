#pragma once
#include <filesystem>
#include "Color.hpp"

namespace gaden
{

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