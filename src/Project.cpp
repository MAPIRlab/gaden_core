#include "gaden/Project.hpp"
#include "YAML_Conversions.hpp"
#include "gaden/internal/PathUtils.hpp"

namespace gaden
{

    Project::Project(std::filesystem::path const& directory)
        : rootDirectory(directory)
    {
    }

    ReadResult Project::Read()
    {
        if (!std::filesystem::exists(rootDirectory))
            return ReadResult::NO_FILE;
        
        try
        {
            envMetadata.ReadFromYAML(rootDirectory / "config.yaml");
            std::vector<std::filesystem::path> simConfigs = paths::GetAllFilesInDirectory(rootDirectory / "simulations");

            for (std::filesystem::path const& sim : simConfigs)
            {
                if (auto params = ParseSimulationFolder(sim))
                {
                    params->simulationID = sim.filename();
                    simulations[params->simulationID] = params.value();
                    GADEN_INFO("Found simulation configuration: {}", params->simulationID);
                }
            }
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Caught exception while reading project directory: '{}'", e.what());
            return ReadResult::READING_FAILED;
        }

        return ReadResult::OK;
    }

    std::optional<Project::SimulationParams> Project::ParseSimulationFolder(std::filesystem::path const& path)
    {
        if (!std::filesystem::is_directory(path))
            return std::nullopt;

        std::filesystem::path yamlPath = path / "sim.yaml";
        if (!std::filesystem::exists(yamlPath))
            return std::nullopt;

        SimulationParams params;
        params.ReadFromYAML(yamlPath);

        return params;
    }

    static void processModelList(YAML::Node const& modelsYAML, std::vector<Model3D>& modelsList, std::filesystem::path const& projectRoot)
    {
        // parse the list of 3d models, and the (optional) corresponding colors for visualization
        Color lastParsedColor;
        for (size_t i = 0; i < modelsYAML.size(); i++)
        {
            std::string entry = modelsYAML[i].as<std::string>();
            if (entry.find("!color") != std::string::npos)
            {
                lastParsedColor = Color::Parse(entry);
                continue;
            }
            std::filesystem::path modelPath = paths::MakeAbsolutePath(entry, projectRoot);
            Model3D model{.path = modelPath, .color = lastParsedColor};
            modelsList.push_back(model);
        }
    }

    void Project::EnvConfigurationMetadata::ReadFromYAML(std::filesystem::path const& yamlPath)
    {
        YAML::Node yaml = YAML::LoadFile(yamlPath);

        // clang-format off
        YAML::Node models_YAML           = yaml["models"];
        YAML::Node outlets_models_YAML   = yaml["outlets_models"];
        
        std::string wind_files;
        FromYAML<std::string> ( yaml, "unprocessed_wind_files", wind_files);
        
        FromYAML<float> ( yaml, "empty_point_x", emptyPoint.x);
        FromYAML<float> ( yaml, "empty_point_y", emptyPoint.y);
        FromYAML<float> ( yaml, "empty_point_z", emptyPoint.z);
        
        FromYAML<float> ( yaml, "cell_size",     cellSize);
        FromYAML<bool>  ( yaml, "uniformWind",   uniformWind);

        // clang-format on

        processModelList(models_YAML, envModels, yamlPath.parent_path());
        processModelList(outlets_models_YAML, outletModels, yamlPath.parent_path());

        unprocessedWindFilePaths = paths::GetWindFiles(paths::MakeAbsolutePath(wind_files, yamlPath.parent_path()));
    }

    std::vector<std::filesystem::path> Project::EnvConfigurationMetadata::GetPaths(std::vector<Model3D> const& models)
    {
        std::vector<std::filesystem::path> paths;
        paths.reserve(models.size());
        for (const auto& m : models)
            paths.push_back(m.path);
        return paths;
    }

} // namespace gaden
