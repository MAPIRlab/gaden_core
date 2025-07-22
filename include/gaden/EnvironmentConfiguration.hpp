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
        
        std::vector<Vector3> localAirflowDisturbances; // small-scale changes to airflow that happen at runtime.
                                                       // To be modified from the outside according to whatever model the user code wants to employ.
                                                       // See AirflowDisturbance.hpp

        bool WriteToDirectory(const std::filesystem::path& path);
        static std::shared_ptr<EnvironmentConfiguration> ReadDirectory(const std::filesystem::path& path);
    };

} // namespace gaden