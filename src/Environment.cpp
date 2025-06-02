#include <fstream>
#include <gaden/Environment.hpp>
#include <gaden/core/Logging.hpp>
#include <gaden/internal/MathUtils.hpp>
#include <yaml-cpp/yaml.h>

namespace gaden
{

    size_t Environment::numCells() const
    {
        return description.dimensions.x * description.dimensions.y * description.dimensions.z;
    }

    size_t Environment::indexFrom3D(Vector3i indices) const
    {
        return ::gaden::indexFrom3D(indices, description.dimensions);
    }

    bool Environment::IsInBounds(const Vector3& point) const
    {
        return IsInBounds(coordsToIndices(point));
    }

    bool Environment::IsInBounds(const Vector3i& indices) const
    {
        return InRange(indices.x, 0, description.dimensions.x) && //
               InRange(indices.y, 0, description.dimensions.y) && //
               InRange(indices.z, 0, description.dimensions.z);
    }

    Environment::CellState& Environment::atRef(const Vector3i& indices) const
    {
        return (CellState&)cells.at(indexFrom3D(indices));
    }

    Environment::CellState& Environment::atRef(const Vector3& point) const
    {
        return atRef(coordsToIndices(point));
    }

    Environment::CellState Environment::at(const Vector3i& indices) const
    {
        if (!IsInBounds(indices))
            return CellState::OutOfBounds;
        return (CellState&)cells.at(indexFrom3D(indices));
    }

    Environment::CellState Environment::at(const Vector3& point) const
    {
        return at(coordsToIndices(point));
    }

    Vector3i Environment::coordsToIndices(const Vector3& coords) const
    {
        return (coords - description.minCoord) / description.cellSize;
    }

    Vector3 Environment::coordsOfCellCenter(const Vector3i& indices) const
    {
        return description.minCoord + (static_cast<Vector3>(indices) + 0.5f) * description.cellSize;
    }

    Vector3 Environment::coordsOfCellOrigin(const Vector3i& indices) const
    {
        return description.minCoord + (static_cast<Vector3>(indices)) * description.cellSize;
    }

    ReadResult Environment::ReadFromFile(const std::filesystem::path& filePath)
    {
        if (!std::filesystem::exists(filePath))
        {
            GADEN_ERROR("Could not find environment file '{}'", filePath);
            return ReadResult::NO_FILE;
        }

        std::ifstream infile(filePath.c_str());
        try
        {
            // open file
            std::string line;

            // read the header
            {
                // Line 1 (min values of environment)
                std::getline(infile, line);
                size_t pos = line.find(" ");
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.minCoord.x = atof(line.substr(0, pos).c_str());
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.minCoord.y = atof(line.substr(0, pos).c_str());
                description.minCoord.z = atof(line.substr(pos + 1).c_str());

                // Line 2 (max values of environment)
                std::getline(infile, line);
                pos = line.find(" ");
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.maxCoord.x = atof(line.substr(0, pos).c_str());
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.maxCoord.y = atof(line.substr(0, pos).c_str());
                description.maxCoord.z = atof(line.substr(pos + 1).c_str());

                // Line 3 (Num cells on eahc dimension)
                std::getline(infile, line);
                pos = line.find(" ");
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.dimensions.x = atoi(line.substr(0, pos).c_str());
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.dimensions.y = atof(line.substr(0, pos).c_str());
                description.dimensions.z = atof(line.substr(pos + 1).c_str());

                // Line 4 cell_size (m)
                std::getline(infile, line);
                pos = line.find(" ");
                description.cellSize = atof(line.substr(pos + 1).c_str());
            }

            cells.resize(description.dimensions.x * description.dimensions.y * description.dimensions.z, CellState::Uninitialized);

            int x_idx = 0;
            int y_idx = 0;
            int z_idx = 0;

            while (std::getline(infile, line))
            {
                std::stringstream ss(line);
                if (z_idx >= description.dimensions.z)
                {
                    printf("Too many lines! z_idx=%d but num_cells_z=%d", z_idx, description.dimensions.z);
                    return ReadResult::READING_FAILED;
                }

                if (line == ";")
                {
                    // New Z-layer
                    z_idx++;
                    x_idx = 0;
                    y_idx = 0;
                }
                else
                { // New line with constant x_idx and all the y_idx values
                    while (ss)
                    {
                        int f;
                        ss >> std::skipws >> f;
                        if (!ss.fail())
                        {
                            cells[indexFrom3D(Vector3i(x_idx, y_idx, z_idx))] = static_cast<CellState>(f);
                            y_idx++;
                        }
                    }

                    // Line has ended
                    x_idx++;
                    y_idx = 0;
                }
            }
        }
        catch (const std::exception& e)
        {
            GADEN_ERROR("Exception caught while trying to parse environment file '{}': '{}'", filePath.c_str(), e.what());
        }

        infile.close();
        return ReadResult::OK;
    }

    bool Environment::WriteToFile(const std::filesystem::path& path)
    {
        std::ofstream outfile(path.c_str());
        if (!outfile.is_open())
        {
            GADEN_ERROR("Could not create output file '{}'", path.c_str());
            return false;
        }

        outfile << "#env_min(m) " << description.minCoord.x << " " << description.minCoord.y << " " << description.minCoord.z << "\n";
        outfile << "#env_max(m) " << description.maxCoord.x << " " << description.maxCoord.y << " " << description.maxCoord.z << "\n";
        outfile << "#num_cells " << description.dimensions.x << " " << description.dimensions.y << " " << description.dimensions.z << "\n";
        outfile << "#cell_size(m) " << description.cellSize << "\n";
        // things are repeated to scale them up (the image is too small!)
        for (int height = 0; height < description.dimensions.z; height++)
        {
            for (int col = 0; col < description.dimensions.x; col++)
            {
                for (int row = 0; row < description.dimensions.y; row++)
                {
                    CellState state = atRef(Vector3i{col, row, height});
                    outfile << (state == CellState::Free ? 0
                                                         : (state == CellState::Outlet ? 2
                                                                                       : 1))
                            << " ";
                }
                outfile << "\n";
            }
            outfile << ";\n";
        }
        outfile.close();

        return true;
    }

    bool Environment::Write2DSlicePGM(const std::filesystem::path& path, float floorHeight, bool blockOutlets)
    {
        try
        {
            int height = (floorHeight - description.minCoord.z) / description.cellSize; // a xy slice of the 3D environment is used as a geometric map for navigation
            if (height < 0 || height >= description.dimensions.z)
            {
                GADEN_ERROR("Cannot print the occupancy map at height {} -- the environment only gets from height {} to {}", height, description.minCoord.z, description.maxCoord.z);
                return false;
            }

            std::ofstream outfile(path.c_str());
            outfile << "P2\n"
                    << description.dimensions.x << " " << description.dimensions.y << "\n"
                    << "1\n";

            for (int row = description.dimensions.y - 1; row >= 0; row--)
            {
                for (int col = 0; col < description.dimensions.x; col++)
                {
                    auto& cell = atRef(Vector3i{col, row, height});
                    bool outletTerm = cell == CellState::Outlet && !blockOutlets;
                    outfile << (cell == CellState::Free || outletTerm ? 1 : 0) << " ";
                }
                outfile << "\n";
            }
            outfile.close();
        }
        catch (const std::exception& e)
        {
            GADEN_ERROR("Error while trying to write 2D Slice at '{}': '{}'", path.c_str(), e.what());
            return false;
        }
        return true;
    }

    bool Environment::WriteROSOccupancyYAML(const std::filesystem::path& path, float height)
    {
        try
        {
            std::ofstream file(path);
            YAML::Emitter yaml;
            yaml.SetDoublePrecision(3);
            yaml << YAML::BeginMap;
            yaml << YAML::Key << "image" << YAML::Value << "occupancy.pgm";
            yaml << YAML::Key << "resolution" << YAML::Value << description.cellSize;

            yaml << YAML::Key << "origin" << YAML::Value << YAML::Flow << std::vector<float>{description.minCoord.x, description.minCoord.y, 0.0}; // the third component is yaw, not Z!
            yaml << YAML::Key << "occupied_thresh" << YAML::Value << 0.9;
            yaml << YAML::Key << "free_thresh" << YAML::Value << 0.1;
            yaml << YAML::Key << "negate" << YAML::Value << 0;

            yaml << YAML::EndMap;
            file << yaml.c_str();
            file.close();
        }
        catch (const std::exception& e)
        {
            GADEN_ERROR("Error while trying to write Occupancy YAML at '{}': '{}'", path.c_str(), e.what());
            return false;
        }
        return true;
    }

    bool Environment::printBasicSimYaml(const std::filesystem::path& path, Vector3 startingPoint)
    {
        try
        {
            std::ofstream file(path.c_str());
            YAML::Emitter yaml;
            yaml.SetDoublePrecision(2);
            yaml << YAML::BeginMap;
            yaml << YAML::Key << "map" << YAML::Value << "occupancy.yaml";
            yaml << YAML::Key << "robots" << YAML::BeginSeq;

            // robot entry
            {
                yaml << YAML::BeginMap;
                yaml << YAML::Key << "name" << YAML::Value << "PioneerP3DX";
                yaml << YAML::Key << "radius" << YAML::Value << 0.25;

                yaml << YAML::Key << "position" << YAML::Value << YAML::Flow
                     << YAML::BeginSeq
                     << startingPoint.x << startingPoint.y << startingPoint.z
                     << YAML::EndSeq;

                yaml << YAML::Key << "angle" << YAML::Value << 0.0 << YAML::Comment("in radians");
                yaml << YAML::Key << "sensors" << YAML::BeginSeq;

                // sensor entry
                {
                    yaml << YAML::BeginMap;
                    yaml << YAML::Key << "type" << YAML::Value << "laser";
                    yaml << YAML::Key << "name" << YAML::Value << "laser_scanner";
                    yaml << YAML::Key << "minAngleRad" << YAML::Value << -2.2;
                    yaml << YAML::Key << "maxAngleRad" << YAML::Value << 2.2;
                    yaml << YAML::Key << "angleResolutionRad" << YAML::Value << 0.07;
                    yaml << YAML::Key << "minDistance" << YAML::Value << 0.1;
                    yaml << YAML::Key << "maxDistance" << YAML::Value << 4.0;
                    yaml << YAML::EndMap;
                }
                yaml << YAML::EndSeq;

                yaml << YAML::EndMap;
            }
            yaml << YAML::EndSeq;

            yaml << YAML::EndMap;

            file << yaml.c_str();
            file.close();
        }
        catch (const std::exception& e)
        {
            GADEN_ERROR("Error while trying to write BasicSim YAML at '{}': '{}'", path.c_str(), e.what());
            return false;
        }
        return true;
    }
} // namespace gaden