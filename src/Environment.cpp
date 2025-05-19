#include <fstream>
#include <gaden/Environment.hpp>
#include <gaden/core/Logging.hpp>
#include <gaden/core/Utils.hpp>

namespace gaden
{

    size_t Environment::numCells() const
    {
        return description.dimensions.x * description.dimensions.y * description.dimensions.z;
    }

    Environment::CellState& Environment::at(const Vector3i& indices)
    {
        return (CellState&)Env[indexFrom3D(indices, description.dimensions)];
    }

    Environment::CellState& Environment::at(const Vector3& point)
    {
        return at(coordsToIndices(point));
    }

    Vector3i Environment::coordsToIndices(const Vector3& coords) const
    {
        return (coords - description.min_coord) / description.cell_size;
    }

    Vector3 Environment::coordsOfCellCenter(const Vector3i& indices) const
    {
        return description.min_coord + (static_cast<Vector3>(indices) + 0.5f) * description.cell_size;
    }

    Vector3 Environment::coordsOfCellOrigin(const Vector3i& indices) const
    {
        return description.min_coord + (static_cast<Vector3>(indices)) * description.cell_size;
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
                description.min_coord.x = atof(line.substr(0, pos).c_str());
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.min_coord.y = atof(line.substr(0, pos).c_str());
                description.min_coord.z = atof(line.substr(pos + 1).c_str());

                // Line 2 (max values of environment)
                std::getline(infile, line);
                pos = line.find(" ");
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.max_coord.x = atof(line.substr(0, pos).c_str());
                line.erase(0, pos + 1);
                pos = line.find(" ");
                description.max_coord.y = atof(line.substr(0, pos).c_str());
                description.max_coord.z = atof(line.substr(pos + 1).c_str());

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
                description.cell_size = atof(line.substr(pos + 1).c_str());
            }

            Env.resize(description.dimensions.x * description.dimensions.y * description.dimensions.z);

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
                            Env[indexFrom3D(Vector3i(x_idx, y_idx, z_idx), description.dimensions)] = static_cast<CellState>(f);
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
} // namespace gaden