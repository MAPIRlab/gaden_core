#pragma once

#include "gaden/core/Vectors.hpp"

namespace gaden
{
    struct Filament
    {
        Vector3 position;
        float sigma;
        bool active = true;
    };
}