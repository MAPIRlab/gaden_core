#pragma once

#include "gaden/RunningSimulation.hpp"
#include <filesystem>
#include "gaden/PlaybackScene.hpp"
#include <map>
#include <optional>
#include <string>

namespace gaden
{

    class EnvironmentConfigMetadata
    {
        using SimulationParams = RunningSimulation::Parameters;

    public:

        EnvironmentConfigMetadata(std::filesystem::path const& directory);
        ReadResult ReadDirectory();
        void WriteConfigYAML(std::filesystem::path const& projectRoot);
        std::string GetName();
        bool CreateTemplate();
        static std::vector<std::filesystem::path> GetPaths(std::vector<Model3D> const& models);
        std::vector<std::filesystem::path> GetWindFiles() const;
        std::filesystem::path GetSimulationFilePath(std::string_view name) { return rootDirectory / "simulations" / name / "sim.yaml"; }
        std::filesystem::path GetConfigFilePath() { return rootDirectory / "config.yaml"; }

    public:
        std::vector<Model3D> envModels;
        std::vector<Model3D> outletModels;

        float cellSize = 0.1f;
        gaden::Vector3 emptyPoint = {0, 0, 0};
        bool uniformWind = false;
        std::string unprocessedWindFiles; // the path, as appears in the configuration file (without the _i.csv suffix)

        std::map<std::string, SimulationParams> simulations;
        std::map<std::string, PlaybackSceneMetadata> playbacks;
        std::filesystem::path rootDirectory;

    private:
        std::optional<SimulationParams> ParseSimulationFolder(std::filesystem::path const& path);
        void ReadFromYAML(std::filesystem::path const& yamlPath);

    private:
    };
} // namespace gaden