#pragma once

#include "gaden/Environment.hpp"
#include "gaden/core/Logging.hpp"
#include "gaden/datatypes/Model3D.hpp"
#include "gaden/internal/WindSequence.hpp"
#include <optional>

namespace gaden
{
    // You can have multiple simulations (source positions, gas types, etc)
    // in the same environment configuration (geometry and airflow)
    struct EnvironmentConfiguration
    {
        Environment environment;
        WindSequence windSequence;

        bool WriteToDirectory(const std::filesystem::path& path);
        static std::optional<EnvironmentConfiguration> ReadDirectory(const std::filesystem::path& path);
    };

} // namespace gaden