#include "gaden/EnvironmentConfiguration.hpp"
#include <gaden/internal/PathUtils.hpp>

namespace gaden
{
    bool EnvironmentConfiguration::ReadDirectory(const std::filesystem::path& directory)
    {
        if (!std::filesystem::is_directory(directory))
        {
            GADEN_ERROR("Path '{}' is not a directory.", directory.c_str());
            return false;
        }

        std::filesystem::path envPath = directory / "Environment.csv";
        if (environment.ReadFromFile(envPath) == ReadResult::NO_FILE)
        {
            GADEN_WARN("Could not read environment file '{}'", envPath.c_str());
            return false;
        }

        std::vector<std::filesystem::path> windFiles = paths::GetAllFilesInDirectory(directory / "wind");
        if (windFiles.empty())
            GADEN_WARN("No wind files in directory '{}'", directory.c_str());

        windSequence.Initialize(windFiles, environment.numCells(), {}); // defaults to no looping

        return true;
    }

    bool EnvironmentConfiguration::WriteToDirectory(const std::filesystem::path& path)
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
