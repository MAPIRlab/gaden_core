#include <fstream>
#include <gaden/Environment.hpp>
#include <gaden/core/Logging.hpp>
#include <gaden/internal/Utils.hpp>

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

    Environment::CellState& Environment::at(const Vector3i& indices) const
    {
        return (CellState&)cells.at(indexFrom3D(indices));
    }

    Environment::CellState& Environment::at(const Vector3& point) const
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
            return ReadResult::NO_FILE;

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

            cells.resize(description.dimensions.x * description.dimensions.y * description.dimensions.z);

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
                        uint8_t f;
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
                    CellState state = at(Vector3i{col, row, height});
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
} // namespace gaden