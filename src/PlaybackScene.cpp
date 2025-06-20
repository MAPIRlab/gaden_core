#include "gaden/PlaybackScene.hpp"
#include "YAML_Conversions.hpp"

namespace gaden
{
    void PlaybackSceneMetadata::ReadFromYAML(std::filesystem::path const& yamlPath, std::filesystem::path const& EnvConfigurationMetadataRoot)
    {
        YAML::Node yaml = YAML::LoadFile(yamlPath);

        size_t startIteration = yaml["initial_iteration"].as<size_t>();
        loop = ParseLoopYAML(yaml["playback_loop"]);
        YAML::Node simulations = yaml["simulations"];

        params.resize(simulations.size());
        gasDisplayColors.resize(simulations.size());
        for (size_t i = 0; i < simulations.size(); i++)
        {
            params.at(i).startIteration = startIteration;
            params.at(i).resultsDirectory = EnvConfigurationMetadataRoot / "simulations" / simulations[i]["sim"].as<std::string>() / "result";

            auto color_vec = simulations[i]["gas_color"].as<std::vector<float>>();
            gasDisplayColors.at(i).r = color_vec[0];
            gasDisplayColors.at(i).g = color_vec[1];
            gasDisplayColors.at(i).b = color_vec[2];
        }
    }

    // currently the colors are only used by the gaden frontend, so they are not stored here at all.
    //  If we create a visualization for the gas dispersion in gaden_gui, we'll need to change that
    PlaybackScene::PlaybackScene(PlaybackSceneMetadata const& metadata, EnvironmentConfiguration const& env)
    {
        GADEN_VERIFY(!metadata.params.empty(), "Cannot create an empty playback scene");
        for (size_t i = 0; i < metadata.params.size(); i++)
            simulations.emplace_back(metadata.params.at(i), env, metadata.loop);
    }

    void PlaybackScene::AdvanceTimestep()
    {
        for (auto& sim : simulations)
            sim.AdvanceTimestep();
    }

    Vector3 PlaybackScene::SampleWind(Vector3 const& point) const
    {
        // wind must be identical for all simulations in the scene (shared environment configuration)
        return simulations.at(0).SampleWind(point);
    }

    std::map<GasType, float> PlaybackScene::SampleConcentrations(Vector3 const& point) const
    {
        std::map<GasType, float> gases;
        for (const auto& sim : simulations)
        {
            GasType type = sim.simulationMetadata.gasType;
            float concentration = 0;
            if (gases.contains(type))
                concentration = gases.at(type);
            gases[type] = concentration + sim.SampleConcentration(point);
        }

        return gases;
    }

    std::vector<GasType> PlaybackScene::GetGasTypes()
    {
        std::vector<GasType> types;
        for (auto& sim : simulations)
            types.push_back(sim.simulationMetadata.gasType);
        return types;
    }

    std::vector<PlaybackSimulation> const& PlaybackScene::GetSimulations()
    {
        return simulations;
    }

    std::vector<gaden::Color> const& PlaybackScene::GetColors()
    {
        return gasDisplayColors;
    }

} // namespace gaden