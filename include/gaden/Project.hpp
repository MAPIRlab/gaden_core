#pragma once

#include "gaden/RunningSimulation.hpp"
#include <filesystem>
#include <map>
#include <string>
#include <optional>

namespace gaden
{

    class Project
    {
        using SimulationParams = RunningSimulation::Parameters;

    public:
        ReadResult Read();

        // data required to generate the internal representation of the environment configuration
        struct EnvConfigurationMetadata
        {
            std::vector<Model3D> envModels;
            std::vector<Model3D> outletModels;
            std::vector<std::filesystem::path> unprocessedWindFilePaths;

            float cellSize = 0.1f;
            gaden::Vector3 emptyPoint = {0, 0, 0};
            bool uniformWind = false;

            void ReadFromYAML(std::filesystem::path const& yamlPath);
            static std::vector<std::filesystem::path> GetPaths(std::vector<Model3D> const& models);
        };

        Project(std::filesystem::path const& directory);

    public:
        EnvConfigurationMetadata envMetadata;
        std::map<std::string, SimulationParams> simulations;

    private:
        std::optional<SimulationParams> ParseSimulationFolder(std::filesystem::path const& path);

    private:
        std::filesystem::path rootDirectory;
    };
} // namespace gaden