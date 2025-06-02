#pragma once

#include "gaden/Environment.hpp"
#include "gaden/core/Logging.hpp"
#include "gaden/datatypes/Model3D.hpp"
#include "gaden/internal/WindSequence.hpp"

namespace gaden
{
    // You can have multiple simulations (source positions, gas types, etc)
    // in the same environment configuration (geometry and airflow)
    struct EnvironmentConfiguration
    {
        Environment environment;
        WindSequence windSequence;
        std::vector<Model3D> visualizationModels; // optional data about how to visualize this environment

        bool WriteToDirectory(const std::filesystem::path& path);
        bool ReadDirectory(const std::filesystem::path& path);
    };

} // namespace gaden