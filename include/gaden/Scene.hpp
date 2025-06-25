#pragma once
#include "PlaybackSimulation.hpp"
#include "gaden/RunningSimulation.hpp"
#include <map>

namespace gaden
{

    struct PlaybackSceneMetadata
    {
        std::vector<PlaybackSimulation::Parameters> params;
        std::vector<Color> gasDisplayColors;
        LoopConfig loop;
        void ReadFromYAML(std::filesystem::path const& path, std::filesystem::path const& projectRoot);
    };

    struct RunningSceneMetadata
    {
        std::vector<RunningSimulation::Parameters> params;
        std::vector<Color> gasDisplayColors;
        void ReadFromYAML(std::filesystem::path const& path, std::filesystem::path const& projectRoot);
    };

    class Scene
    {
    public:
        Scene(PlaybackSceneMetadata const& metadata, EnvironmentConfiguration const& env);
        Scene(RunningSceneMetadata const& metadata, EnvironmentConfiguration const& env);
        void AdvanceTimestep();
        Vector3 SampleWind(Vector3 const& point) const;
        std::map<GasType, float> SampleConcentrations(Vector3 const& point) const;
        std::vector<GasType> GetGasTypes();

        std::vector<std::shared_ptr<Simulation>> const& GetSimulations();
        std::vector<gaden::Color> GetColors();

    private:
        std::vector<std::shared_ptr<Simulation>> simulations;
    };
} // namespace gaden