#pragma once
#include "PlaybackSimulation.hpp"
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

    class PlaybackScene
    {
    public:
        PlaybackScene(PlaybackSceneMetadata const& metadata, EnvironmentConfiguration const& env);
        void AdvanceTimestep();
        Vector3 SampleWind(Vector3 const& point) const;
        std::map<GasType, float> SampleConcentrations(Vector3 const& point) const;
        std::vector<GasType> GetGasTypes();

        std::vector<PlaybackSimulation> const& GetSimulations();
        std::vector<gaden::Color> const& GetColors();

    private:
        std::vector<gaden::Color> gasDisplayColors;
        std::vector<PlaybackSimulation> simulations;
    };
} // namespace gaden