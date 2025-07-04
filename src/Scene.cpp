#include "gaden/Scene.hpp"
#include "YAML_Conversions.hpp"
#include <fstream>

namespace gaden
{
    void PlaybackSceneMetadata::ReadFromYAML(std::filesystem::path const& yamlPath, std::filesystem::path const& EnvConfigurationMetadataRoot)
    {
        YAML::Node yaml = YAML::LoadFile(yamlPath);

        size_t startIteration = yaml["playback_initial_iteration"].as<size_t>();
        loop = ParseLoopYAML(yaml["playback_loop"]);
        YAML::Node simulations = yaml["simulations"];

        params.resize(simulations.size());
        gasDisplayColors.resize(simulations.size());
        for (size_t i = 0; i < simulations.size(); i++)
        {
            params.at(i).startIteration = startIteration;
            params.at(i).resultsDirectory = EnvConfigurationMetadataRoot / "simulations" / simulations[i]["sim"].as<std::string>() / "result";

            Vector3 color_vec{0.4, 0.4, 0.4}; // default
            if (YAML::Node node = simulations[i]["gas_color"])
                color_vec = node.as<Vector3>();

            gasDisplayColors.at(i).r = color_vec[0];
            gasDisplayColors.at(i).g = color_vec[1];
            gasDisplayColors.at(i).b = color_vec[2];
            gasDisplayColors.at(i).a = 1;
        }
    }

    void PlaybackSceneMetadata::WriteToYAML(std::filesystem::path const& path)
    {
        std::ofstream file(path);
        YAML::Emitter emitter(file);
        emitter << YAML::BeginMap;

        emitter << YAML::Key << "playback_initial_iteration" << YAML::Value;
        if (params.empty())
            emitter << 0;
        else
            emitter << params.at(0).startIteration;

        emitter << YAML::Key << "playback_loop" << YAML::Value;
        WriteLoopYAML(emitter, loop);

        emitter << YAML::Key << "simulations" << YAML::Value << YAML::Block << YAML::BeginSeq;
        for (size_t i = 0; i < params.size(); i++)
        {
            emitter << YAML::BeginMap;
            std::string name = params.at(i).resultsDirectory.parent_path().stem();
            emitter << YAML::Key << "sim" << YAML::Value << name;
            emitter << YAML::Key << "gas_color" << YAML::Value << YAML::Flow << Vector3(.4, .4, .4);
            emitter << YAML::EndMap;
        }
        emitter << YAML::EndSeq;

        emitter << YAML::EndMap;

        file.close();
    }

    void RunningSceneMetadata::ReadFromYAML(std::filesystem::path const& path, std::filesystem::path const& projectRoot)
    {
        YAML::Node yaml = YAML::LoadFile(path);

        size_t startIteration = yaml["playback_initial_iteration"].as<size_t>();
        LoopConfig loop = ParseLoopYAML(yaml["playback_loop"]);
        YAML::Node simulations = yaml["simulations"];

        params.resize(simulations.size());
        gasDisplayColors.resize(simulations.size());
        for (size_t i = 0; i < simulations.size(); i++)
        {
            std::string filePath = projectRoot / "simulations" / simulations[i]["sim"].as<std::string>() / "sim.yaml";
            params.at(i).ReadFromYAML(filePath);

            Vector3 color_vec{0.4, 0.4, 0.4}; // default
            if (YAML::Node node = simulations[i]["gas_color"])
                color_vec = node.as<Vector3>();

            gasDisplayColors.at(i).r = color_vec[0];
            gasDisplayColors.at(i).g = color_vec[1];
            gasDisplayColors.at(i).b = color_vec[2];
            gasDisplayColors.at(i).a = 1;
        }
    }

    Scene::Scene(PlaybackSceneMetadata const& metadata, EnvironmentConfiguration const& env)
    {
        GADEN_VERIFY(!metadata.params.empty(), "Cannot create an empty playback scene");
        for (size_t i = 0; i < metadata.params.size(); i++)
        {
            simulations.push_back(std::make_shared<PlaybackSimulation>(metadata.params.at(i), env, metadata.loop));
            auto& sim = simulations.back();
            sim->gasDisplayColor = metadata.gasDisplayColors.at(i);
        }
    }

    Scene::Scene(RunningSceneMetadata const& metadata, EnvironmentConfiguration const& env)
    {
        GADEN_VERIFY(!metadata.params.empty(), "Cannot create an empty running scene");
        for (size_t i = 0; i < metadata.params.size(); i++)
        {
            simulations.push_back(std::make_shared<RunningSimulation>(metadata.params.at(i), env));
            auto& sim = simulations.back();
            sim->gasDisplayColor = metadata.gasDisplayColors.at(i);
        }
    }

    void Scene::AdvanceTimestep()
    {
        for (auto& sim : simulations)
            sim->AdvanceTimestep();
    }

    Vector3 Scene::SampleWind(Vector3 const& point) const
    {
        // wind must be identical for all simulations in the scene (shared environment configuration)
        return simulations.at(0)->SampleWind(point);
    }

    std::map<GasType, float> Scene::SampleConcentrations(Vector3 const& point) const
    {
        std::map<GasType, float> gases;
        for (const auto& sim : simulations)
        {
            GasType type = sim->simulationMetadata.source->gasType;
            float concentration = 0;
            if (gases.contains(type))
                concentration = gases.at(type);
            gases[type] = concentration + sim->SampleConcentration(point);
        }

        return gases;
    }

    std::vector<GasType> Scene::GetGasTypes()
    {
        std::vector<GasType> types;
        for (auto& sim : simulations)
            types.push_back(sim->simulationMetadata.source->gasType);
        return types;
    }

    std::vector<std::shared_ptr<Simulation>> const& Scene::GetSimulations()
    {
        return simulations;
    }

    std::vector<gaden::Color> Scene::GetColors()
    {
        std::vector<Color> colors;
        for (auto& sim : simulations)
            colors.push_back(sim->gasDisplayColor);
        return colors;
    }

} // namespace gaden