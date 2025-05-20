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
        std::filesystem::path path;

        static EnvironmentConfiguration ReadDirectory(const std::filesystem::path& path);
    };

    inline EnvironmentConfiguration EnvironmentConfiguration::ReadDirectory(const std::filesystem::path& directory)
    {
        EnvironmentConfiguration config;
        if (!std::filesystem::is_directory(directory))
        {
            GADEN_ERROR("Path '{}' is not a directory.");
            return config;
        }

        config.path = directory;
        config.environment.ReadFromFile(directory / "Environment.csv");

        std::vector<std::filesystem::path> windFiles;
        for (const auto& file : std::filesystem::directory_iterator(directory / "wind"))
            windFiles.push_back(file);
        config.windSequence.Initialize(windFiles, config.environment.numCells(), {}); // defaults to no looping
        
        return config;
    }
} // namespace gaden