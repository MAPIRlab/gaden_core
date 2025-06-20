#include "gaden/EnvironmentConfigMetadata.hpp"
#include "YAML_Conversions.hpp"
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
            std::vector<std::filesystem::path> playbackConfigs = paths::GetAllFilesInDirectory(rootDirectory / "playbacks");
            for (std::filesystem::path const& playbackFile : playbackConfigs)
            {
                PlaybackSceneMetadata metadata;
                metadata.ReadFromYAML(playbackFile, rootDirectory);
                playbacks[playbackFile.stem()] = metadata;
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
        unprocessedWindFiles = paths::MakeAbsolutePath(wind_files, yamlPath.parent_path());
        
        FromYAML<float> ( yaml, "empty_point_x", emptyPoint.x);
        FromYAML<float> ( yaml, "empty_point_y", emptyPoint.y);
        FromYAML<float> ( yaml, "empty_point_z", emptyPoint.z);
        
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
        emitter << YAML::Key << "empty_point_x" << YAML::Value << emptyPoint.x;
        emitter << YAML::Key << "empty_point_y" << YAML::Value << emptyPoint.y;
        emitter << YAML::Key << "empty_point_z" << YAML::Value << emptyPoint.z;

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

    bool EnvironmentConfigMetadata::CreateTemplate()
    {
        try
        {
            GADEN_INFO("Creating environment configuration  at'{}'", rootDirectory);
            // create the root-level stuff
            std::filesystem::create_directories(rootDirectory / "simulations");
            std::filesystem::create_directories(rootDirectory / "playbacks");
            std::ofstream configFile(rootDirectory / "config.yaml");
            configFile <<
                R"(
# CAD models of the enviroment (.stl)
models: 
  - "!color [0.92, 0.96, 0.96]"
  - "path to the model. Can be relative to this file"
  - "path to the model. Can be relative to this file"

# CAD model of the outlets (.stl)
outlets_models: 
  - "!color [0.96, 0.17, 0.3]"
  - "path to the model. Can be relative to this file"
  - "path to the model. Can be relative to this file"

cell_size: 0.1

# 3D Location of a point in free-space
empty_point_x: 0.0      ### (m)
empty_point_y: 0.0      ### (m)
empty_point_z: 0.0      ### (m)

# Wind Data (the node will append _i.csv to the name that is specified here)
uniformWind: false
unprocessed_wind_files: "" #path, relative to this file.  _i.csv automatically appended)";
            configFile.close();

            // create a sample simulation config
            std::filesystem::create_directories(rootDirectory / "simulations" / "sim1");
            std::ofstream simFile(rootDirectory / "simulations" / "sim1" / "sim.yaml");
            simFile <<
                R"(
gasType: 0
sourcePosition: [0.0, 0.0, 0.0]

deltaTime: 0.1                    
numFilaments_sec: 10   
windIterationDeltaTime: 1.0          

temperature: 298.0                 
pressure: 1.0                
filament_ppm_center: 10.0         
filament_initial_std: 10.0         
filament_growth_gamma: 15.0        
filament_noise_std: 0.02           

saveResults: true
saveDeltaTime: 0.5

# Wind Loop options
wind_looping:
  loop: false
  from: 0
  to: 24)";
            simFile.close();

            // create a sample playback file
            std::ofstream playbackFile(rootDirectory / "playbacks" / "scene1.yaml");
            playbackFile <<
                R"(
initial_iteration: 0

playback_loop:
  loop: true
  from: 0
  to: 100

simulations:
  - sim: "sim1"
    gas_color: [0.0, 1.0, 0.0]
)";
            playbackFile.close();
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Failed to create environment configuration template at '{}'", rootDirectory);
            return false;
        }
        return true;
    }

    std::vector<std::filesystem::path> EnvironmentConfigMetadata::GetWindFiles() const
    {
        return paths::GetExternalWindFiles(paths::MakeAbsolutePath(unprocessedWindFiles, rootDirectory));
    }

} // namespace gaden
