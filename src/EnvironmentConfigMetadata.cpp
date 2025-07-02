#include "gaden/EnvironmentConfigMetadata.hpp"
#include "YAML_Conversions.hpp"
#include "gaden/datatypes/sources/PointSource.hpp"
#include "gaden/internal/PathUtils.hpp"
#include <fstream>

namespace gaden
{

    EnvironmentConfigMetadata::EnvironmentConfigMetadata(std::filesystem::path const& directory)
        : rootDirectory(std::filesystem::canonical(directory))
    {
    }

    ReadResult EnvironmentConfigMetadata::ReadDirectory()
    {
        if (!std::filesystem::exists(rootDirectory))
            return ReadResult::NO_FILE;

        try
        {
            // read the environment configuration
            ReadFromYAML(rootDirectory / "config.yaml");
            std::vector<std::filesystem::path> simConfigs = paths::GetAllFilesInDirectory(rootDirectory / "simulations");

            // read the simulation configurations
            for (std::filesystem::path const& sim : simConfigs)
            {
                if (auto params = ParseSimulationFolder(sim))
                {
                    std::string name = sim.stem();
                    simulations[name] = params.value();
                    GADEN_INFO("Found simulation configuration: {}", name);
                }
            }

            // read the playback configurations
            std::vector<std::filesystem::path> playbackConfigs = paths::GetAllFilesInDirectory(rootDirectory / "scenes");
            for (std::filesystem::path const& playbackFile : playbackConfigs)
            {
                PlaybackSceneMetadata metadata;
                metadata.ReadFromYAML(playbackFile, rootDirectory);
                scenes[playbackFile.stem()] = metadata;
                GADEN_INFO("Found playback configuration: {}", playbackFile.stem());
            }
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Caught exception while reading EnvConfigurationMetadata directory: '{}'", e.what());
            return ReadResult::READING_FAILED;
        }

        return ReadResult::OK;
    }

    std::optional<EnvironmentConfigMetadata::SimulationParams> EnvironmentConfigMetadata::ParseSimulationFolder(std::filesystem::path const& path)
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

    static void processModelList(YAML::Node const& modelsYAML, std::vector<Model3D>& modelsList, std::filesystem::path const& EnvConfigurationMetadataRoot)
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
            std::filesystem::path modelPath = paths::MakeAbsolutePath(entry, EnvConfigurationMetadataRoot);
            if (!std::filesystem::exists(modelPath) || modelPath.extension() != ".stl")
            {
                GADEN_WARN("Ignoring path '{}', which is not a valid .stl model", modelPath);
                continue;
            }

            Model3D model{.path = modelPath, .color = lastParsedColor};
            modelsList.push_back(model);
        }
    }

    void EnvironmentConfigMetadata::EnvironmentConfigMetadata::ReadFromYAML(std::filesystem::path const& yamlPath)
    {
        YAML::Node yaml = YAML::LoadFile(yamlPath);

        // clang-format off
        YAML::Node models_YAML           = yaml["models"];
        YAML::Node outlets_models_YAML   = yaml["outlets_models"];
        
        std::string wind_files;
        FromYAML<std::string> ( yaml, "unprocessed_wind_files", wind_files);
        if(wind_files != "")
            unprocessedWindFiles = paths::MakeAbsolutePath(wind_files, yamlPath.parent_path());
        
        FromYAML<Vector3> ( yaml, "empty_point", emptyPoint);
        
        FromYAML<float> ( yaml, "cell_size",     cellSize);
        FromYAML<bool>  ( yaml, "uniformWind",   uniformWind);

        // clang-format on

        processModelList(models_YAML, envModels, yamlPath.parent_path());
        processModelList(outlets_models_YAML, outletModels, yamlPath.parent_path());
    }

    void EnvironmentConfigMetadata::WriteConfigYAML(std::filesystem::path const& projectRoot)
    {
        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "models" << YAML::Value;
        EncodeModelsList(emitter, envModels, projectRoot, GetConfigFilePath());

        emitter << YAML::Key << "outlets_models" << YAML::Value;
        EncodeModelsList(emitter, outletModels, projectRoot, GetConfigFilePath());

        // wind files
        emitter << YAML::Key << "unprocessed_wind_files" << YAML::Value << paths::MakeRelativeIfInProject(unprocessedWindFiles, //
                                                                                                          projectRoot,          //
                                                                                                          GetConfigFilePath().parent_path())
                                                                               .c_str();
        emitter << YAML::Key << "empty_point" << YAML::Value << YAML::Flow << emptyPoint;

        emitter << YAML::Key << "cell_size" << YAML::Value << cellSize;
        emitter << YAML::Key << "uniformWind" << YAML::Value << uniformWind;
        emitter << YAML::EndMap;

        std::filesystem::path path(GetConfigFilePath());
        std::ofstream file(path);
        file << emitter.c_str();
        file.close();
        GADEN_INFO("Wrote configuration to '{}'", path);
    }

    std::string EnvironmentConfigMetadata::GetName()
    {
        return rootDirectory.filename().string();
    }

    std::vector<std::filesystem::path> EnvironmentConfigMetadata::EnvironmentConfigMetadata::GetPaths(std::vector<Model3D> const& models)
    {
        std::vector<std::filesystem::path> paths;
        paths.reserve(models.size());
        for (const auto& m : models)
            paths.push_back(m.path);
        return paths;
    }

    bool EnvironmentConfigMetadata::CreateTemplate(std::filesystem::path const& directory)
    {
        try
        {
            GADEN_INFO("Creating environment configuration  at'{}'", directory);
            // create the root-level stuff
            std::filesystem::create_directories(directory / "simulations" / "sim1");
            std::filesystem::create_directories(directory / "scenes");
            EnvironmentConfigMetadata configMetadata(directory);
            configMetadata.WriteConfigYAML(directory);

            // create a sample simulation config
            std::filesystem::path simFile = configMetadata.GetSimulationFilePath("sim1");
            RunningSimulation::Parameters sim1{.source = std::make_shared<PointSource>(), .saveResults = true};
            sim1.WriteToYAML(simFile);

            // create a sample scene file
            PlaybackSceneMetadata metadata;
            metadata.params.push_back({.resultsDirectory = simFile.parent_path() / "result"});
            metadata.WriteToYAML(directory / "scenes" / "scene1.yaml");
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Failed to create environment configuration template at '{}'", directory);
            return false;
        }
        return true;
    }

    std::vector<std::filesystem::path> EnvironmentConfigMetadata::GetWindFiles() const
    {
        return paths::GetExternalWindFiles(paths::MakeAbsolutePath(unprocessedWindFiles, rootDirectory));
    }

} // namespace gaden
