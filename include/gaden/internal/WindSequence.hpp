#pragma once

#include "gaden/core/ReadResult.hpp"
#include "gaden/core/Vectors.hpp"
#include <filesystem>
namespace gaden
{
    // Holds a list of wind maps and and index that points to the currently active one
    class WindSequence
    {
    public:
        WindSequence(const std::vector<std::filesystem::path>& files, size_t numCells, bool loop);
        std::vector<Vector3>& GetCurrent();
        void AdvanceTimeStep();

    private:
        ReadResult parseFile(const std::filesystem::path& path, std::vector<Vector3>& map);

    private:
        std::vector<std::vector<Vector3>> windMaps;
        size_t indexCurrent;
        bool looping;
    };
} // namespace gaden