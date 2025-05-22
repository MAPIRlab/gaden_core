#pragma once

#include "gaden/Environment.hpp"
#include "gaden/core/Logging.hpp"
#include "gaden/internal/WindSequence.hpp"

namespace gaden
{
    // You can have multiple simulations (source positions, gas types, etc)
    // in the same environment configuration (geometry and airflow)
    struct EnvironmentConfiguration
    {
        Environment environment;
        WindSequence windSequence;
        std::filesystem::path path; // path to the root directory for this configuration. Used to decide where to save results and such

        bool WriteToDirectory();

        static EnvironmentConfiguration ReadDirectory(const std::filesystem::path& path);
    };

} // namespace gaden