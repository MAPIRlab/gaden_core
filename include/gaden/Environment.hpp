#pragma once

#include "core/GadenVersion.hpp"
#include "core/ReadResult.hpp"
#include "core/Vectors.hpp"
#include <cstdint>
#include <filesystem>

namespace gaden
{
    // represents the environment geometry (obstacles and outlets), without any information about gas/wind
    class Environment
    {
    public:
        enum class CellState : uint8_t
        {
            Free = 0,        // cell is empty, gas can be here
            Obstacle = 1,    // cell is occupied by an obstacle, no filaments can go through it
            Outlet = 2,      // if a filament enters this cell it is removed from the simulation
            OutOfBounds = 3, // invalid cell, position is out of the map bounds
            Uninitialized = 255
        };

        struct Description
        {
            Vector3i dimensions;
            Vector3 minCoord; //[m]
            Vector3 maxCoord; //[m]
            float cellSize;   //[m]
        };

    public:
        size_t numCells() const;
        size_t indexFrom3D(Vector3i indices) const;
        Vector3i indicesFrom1D(size_t index) const;

        bool IsInBounds(const Vector3& point) const;
        bool IsInBounds(const Vector3i& indices) const;

        CellState at(const Vector3i& indices) const;
        CellState at(const Vector3& point) const;

        // the reference versions cannot return an OOB value when the indices are wrong! It is the caller's responsibility to check the indices are in bounds first
        CellState& atRef(const Vector3i& indices) const;
        CellState& atRef(const Vector3& point) const;

        Vector3i coordsToIndices(const Vector3& coords) const;
        Vector3 coordsOfCellCenter(const Vector3i& indices) const;
        Vector3 coordsOfCellOrigin(const Vector3i& indices) const;
        ReadResult ReadFromFile(const std::filesystem::path& filePath);

        bool WriteToFile(const std::filesystem::path& path);
        bool Write2DSlicePGM(const std::filesystem::path& path, float height, bool blockOutlets);
        bool WriteROSOccupancyYAML(const std::filesystem::path& path, float height);
        bool printBasicSimYaml(const std::filesystem::path& path, Vector3 startingPoint);

    public:
        int versionMajor = gaden::version_major,
            versionMinor = gaden::version_minor; // version of gaden used to generate a log file. Used to figure out how to parse the binary format

        Description description;

        std::vector<CellState> cells;

    private:
    };

} // namespace gaden