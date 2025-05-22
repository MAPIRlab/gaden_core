#pragma once

#include "gaden/core/ReadResult.hpp"
#include "gaden/core/Vectors.hpp"
#include "gaden/datatypes/LoopConfig.hpp"
#include <filesystem>
namespace gaden
{
    // Holds a list of wind maps and and index that points to the currently active one
    class WindSequence
    {
    public:
        void Initialize(const std::vector<std::filesystem::path>& files, size_t numCells, LoopConfig loopConf);
        void Initialize(const std::vector<std::vector<Vector3>>& windIterations, size_t numCells, LoopConfig loopConf);
        void AdvanceTimeStep();
        std::vector<Vector3>& GetCurrent();
        const std::vector<Vector3>& GetCurrent() const;
        size_t GetCurrentIndex();
        void SetCurrentIndex(size_t index);
        bool WriteToFiles(const std::filesystem::path& directory, std::string_view namePrefix);

    private:
        ReadResult parseFile(const std::filesystem::path& path, std::vector<Vector3>& map);
        void parseModernFile(std::ifstream& infile, std::vector<Vector3>& map);
        void parseOldFile(std::ifstream& infile, std::vector<Vector3>& map);

    public:
        LoopConfig loopConfig;

    private:
        std::vector<std::vector<Vector3>> windMaps;
        size_t indexCurrent;
    };
} // namespace gaden