#pragma once
#include "gaden/datatypes/Model3D.hpp"
#include <vector>

namespace gaden
{
    // requires coppelia to be running on a separate process
    void GenerateCoppeliaScene(std::vector<Model3D> const& models);

} // namespace gaden