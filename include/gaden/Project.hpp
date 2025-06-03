#pragma once

#include "gaden/RunningSimulation.hpp"
#include <filesystem>
#include <gaden/PlaybackSimulation.hpp>
#include <map>
#include <optional>
#include <string>

namespace gaden
{

    class Project
    {
        using SimulationParams = RunningSimulation::Parameters;

    public:
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

        struct PlaybackMetadata
        {
            std::vector<PlaybackSimulation::Parameters> params;
            std::vector<Color> gasDisplayColor;
            LoopConfig loop;
            void ReadFromYAML(std::filesystem::path const& path, std::filesystem::path const& projectRoot);
        };

        Project(std::filesystem::path const& directory);
        ReadResult Read();

    public:
        EnvConfigurationMetadata envMetadata;
        std::map<std::string, SimulationParams> simulations;
        std::map<std::string, PlaybackMetadata> playbacks;
        std::filesystem::path rootDirectory;

    private:
        std::optional<SimulationParams> ParseSimulationFolder(std::filesystem::path const& path);
    };
} // namespace gaden