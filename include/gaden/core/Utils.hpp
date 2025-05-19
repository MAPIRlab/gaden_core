#pragma once
#include "Vectors.hpp"

namespace gaden
{
    inline size_t indexFrom3D(const Vector3i& index, const Vector3i& num_cells_env)
    {
        return index.x + index.y * num_cells_env.x + index.z * num_cells_env.x * num_cells_env.y;
    }
} // namespace gaden