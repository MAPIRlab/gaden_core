#pragma once

#include "gaden/Environment.hpp"
#include "gaden/datatypes/Model3D.hpp"

namespace gaden
{
    void GenerateMVSimScene(Environment const& env, std::vector<Model3D> const& models, std::filesystem::path const& outputFolder);
}