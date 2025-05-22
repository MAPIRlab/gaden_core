#include "gaden/EnvironmentConfiguration.hpp"

namespace gaden
{
    EnvironmentConfiguration EnvironmentConfiguration::ReadDirectory(const std::filesystem::path& directory)
    {
        EnvironmentConfiguration config;
        if (!std::filesystem::is_directory(directory))
        {
            GADEN_ERROR("Path '{}' is not a directory.");
            return config;
        }

        config.path = directory;
        std::filesystem::path envPath = directory / "Environment.csv";
        if (config.environment.ReadFromFile(envPath) == ReadResult::NO_FILE)
            GADEN_ERROR("Could not read environment file '{}'", envPath.c_str());

        std::vector<std::filesystem::path> windFiles;
        if (std::filesystem::exists(directory / "wind"))
            for (const auto& file : std::filesystem::directory_iterator(directory / "wind"))
                windFiles.push_back(file);
        else
            GADEN_WARN("No wind files in directory '{}'", directory.c_str());

        config.windSequence.Initialize(windFiles, config.environment.numCells(), {}); // defaults to no looping

        return config;
    }

    bool EnvironmentConfiguration::WriteToDirectory()
    {
        if (!std::filesystem::exists(path))
        {
            GADEN_INFO("Output directory '{}' does not exist. Trying to create it.", path.c_str());
            if (!std::filesystem::create_directory(path))
            {
                GADEN_ERROR("Failed to create output directory!");
                return false;
            }
        }

        if (!environment.WriteToFile(path / "OccupancyGrid3D.csv"))
            return false;

        std::filesystem::create_directory(path / "wind");
        if (!windSequence.WriteToFiles(path / "wind", "wind_iteration"))
            return false;

        GADEN_INFO("Wrote environment configuration to '{}'", path.c_str());
        return true;
    }
} // namespace gaden
