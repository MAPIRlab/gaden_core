#pragma once

#include "GadenVersion.hpp"
#include "core/ReadResult.hpp"
#include "core/Vectors.hpp"
#include <cstdint>
#include <filesystem>

namespace gaden
{
    class Environment
    {
    public:
        enum class CellState : uint8_t
        {
            Free = 0,       // cell is empty, gas can be here
            Obstacle = 1,   // cell is occupied by an obstacle, no filaments can go through it
            Outlet = 2,     // if a filament enters this cell it is removed from the simulation
            OutOfBounds = 3 // invalid cell, position is out of the map bounds
        };

        struct Description
        {
            Vector3i dimensions;
            Vector3 min_coord; //[m]
            Vector3 max_coord; //[m]
            float cell_size;   //[m]
        };

    public:
        size_t numCells() const;

        Environment::CellState& at(const Vector3i& indices);
        CellState& at(const Vector3& point);

        Vector3i coordsToIndices(const Vector3& coords) const;
        Vector3 coordsOfCellCenter(const Vector3i& indices) const;
        Vector3 coordsOfCellOrigin(const Vector3i& indices) const;
        ReadResult ReadFromFile(const std::filesystem::path& filePath);

    public:
        int versionMajor = gaden::version_major,
            versionMinor = gaden::version_minor; // version of gaden used to generate a log file. Used to figure out how to parse the binary format

        Description description;

        std::vector<CellState> Env;

    private:
    };
} // namespace gaden