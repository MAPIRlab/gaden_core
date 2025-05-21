#pragma once

#include "gaden/core/Vectors.hpp"

namespace gaden
{
    struct Filament
    {
        Filament(float x, float y, float z, float sigma_filament)
            : position(x, y, z), sigma(sigma_filament)
        {}
        Filament(Vector3 pos, float sigma_filament)
            : position(pos), sigma(sigma_filament)
        {}

        Vector3 position;
        float sigma;
        bool active = true;
    };
} // namespace gaden