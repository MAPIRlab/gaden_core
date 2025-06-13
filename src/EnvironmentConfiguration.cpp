#include "gaden/EnvironmentConfiguration.hpp"
#include <gaden/internal/PathUtils.hpp>
#include <optional>

namespace gaden
{
    std::optional<EnvironmentConfiguration> EnvironmentConfiguration::ReadDirectory(const std::filesystem::path& directory)
    {
        if (!std::filesystem::is_directory(directory))
        {
            GADEN_ERROR("Path '{}' is not a directory.", directory.c_str());
            return std::nullopt;
        }
        EnvironmentConfiguration config;

        std::filesystem::path envPath = directory / "OccupancyGrid3D.csv";
        if (config.environment.ReadFromFile(envPath) == ReadResult::NO_FILE)
        {
            GADEN_WARN("Could not read environment file '{}'", envPath.c_str());
            return std::nullopt;
        }

        std::vector<std::filesystem::path> windFiles = paths::GetAllFilesInDirectory(directory / "wind");
        if (windFiles.empty())
            GADEN_WARN("No wind files in directory '{}'", directory.c_str());

        config.windSequence.Initialize(windFiles, config.environment.numCells(), {}); // defaults to no looping

        return config;
    }

    bool EnvironmentConfiguration::WriteToDirectory(const std::filesystem::path& path)
    {
        try
        {
            if (!std::filesystem::exists(path))
            {
                GADEN_INFO("Output directory '{}' does not exist. Trying to create it.", path.c_str());
                if (!std::filesystem::create_directories(path))
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
        catch (std::exception const& e)
        {
            GADEN_ERROR("Error when trying to write environment configuration to '{}'", path);
            return false;
        }
    }
} // namespace gaden
