#pragma once

#include "gaden/RunningSimulation.hpp"
#include <filesystem>
#include <gaden/PlaybackSimulation.hpp>
#include <map>
#include <optional>
#include <string>

namespace gaden
{

    class EnvironmentConfigMetadata
    {
        using SimulationParams = RunningSimulation::Parameters;

    public:
        struct PlaybackMetadata
        {
            std::vector<PlaybackSimulation::Parameters> params;
            std::vector<Color> gasDisplayColor;
            LoopConfig loop;
            void ReadFromYAML(std::filesystem::path const& path, std::filesystem::path const& projectRoot);
        };

        EnvironmentConfigMetadata(std::filesystem::path const& directory);
        ReadResult ReadDirectory();
        void ReadFromYAML(std::filesystem::path const& yamlPath);
        static std::vector<std::filesystem::path> GetPaths(std::vector<Model3D> const& models);
        bool CreateTemplate();

    public:
        std::vector<Model3D> envModels;
        std::vector<Model3D> outletModels;
        std::vector<std::filesystem::path> unprocessedWindFilePaths;

        float cellSize = 0.1f;
        gaden::Vector3 emptyPoint = {0, 0, 0};
        bool uniformWind = false;

        std::map<std::string, SimulationParams> simulations;
        std::map<std::string, PlaybackMetadata> playbacks;
        std::filesystem::path rootDirectory;

    private:
        std::optional<SimulationParams> ParseSimulationFolder(std::filesystem::path const& path);
    };
} // namespace gaden