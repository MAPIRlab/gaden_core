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
        struct LoopConfig
        {
            bool loop = false;
            size_t from = 0;
            size_t to = 0;
        };

    public:
        void Initialize(const std::vector<std::filesystem::path>& files, size_t numCells, LoopConfig loopConf);
        void AdvanceTimeStep();
        std::vector<Vector3>& GetCurrent();
        const std::vector<Vector3>& GetCurrent() const;
        size_t GetCurrentIndex();
        void SetCurrentIndex(size_t index);

    private:
        ReadResult parseFile(const std::filesystem::path& path, std::vector<Vector3>& map);

    public:
        LoopConfig loopConfig;

    private:
        std::vector<std::vector<Vector3>> windMaps;
        size_t indexCurrent;
    };
} // namespace gaden