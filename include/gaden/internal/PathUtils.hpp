#pragma once
#include "gaden/core/Assertions.hpp"
#include <filesystem>
#include <vector>

namespace gaden::paths
{
    inline void TryCreateDirectory(std::filesystem::path const& path)
    {
        if (!std::filesystem::exists(path))
        {
            if (!std::filesystem::create_directory(path))
            {
                GADEN_WARN("Cannot create directory '{}'", path);
                GADEN_TERMINATE;
            }
        }
    }

    inline std::vector<std::filesystem::path> AsPaths(const std::vector<std::string>& strs)
    {
        std::vector<std::filesystem::path> paths;
        paths.reserve(strs.size());
        for (const auto& str : strs)
            paths.emplace_back(str);
        return paths;
    }

    inline std::vector<std::filesystem::path> GetAllFilesInDirectory(std::filesystem::path const& directory)
    {
        std::vector<std::filesystem::path> files;
        if (std::filesystem::exists(directory))
            for (const auto& file : std::filesystem::directory_iterator(directory))
                files.push_back(file);
        return files;
    }

    inline std::filesystem::path MakeAbsolutePath(std::filesystem::path const& path, std::filesystem::path const& referencePoint)
    {
        if (!path.is_relative())
            return path;

        return referencePoint / path;
    }

    inline std::vector<std::filesystem::path> GetExternalWindFiles(std::string const& commonPath)
    {
        std::string filename = fmt::format("{}_{}.csv", commonPath, 0);
        if (!std::filesystem::exists(filename))
        {
            GADEN_WARN("Wind file '{}' does not exist", filename.c_str());
            return {};
        }

        size_t idx = 0;
        std::vector<std::filesystem::path> paths;
        for (; std::filesystem::exists(filename); filename = fmt::format("{}_{}.csv", commonPath, idx))
        {
            paths.emplace_back(filename);
            idx++;
        }
        return paths;
    }
} // namespace gaden::paths