#include "gaden/core/Logging.hpp"
#include <fstream>
#include <gaden/internal/WindSequence.hpp>

namespace gaden
{

    WindSequence::WindSequence(const std::vector<std::filesystem::path>& files, size_t numCells, bool loop)
        : indexCurrent(0), looping(loop)
    {
        windMaps.resize(files.size(), std::vector<Vector3>(numCells));
        for (size_t i = 0; i < files.size(); i++)
        {
            const auto& file = files[i];
            if (parseFile(file, windMaps[i]) != ReadResult::OK)
                GADEN_TERMINATE;
        }
    }

    std::vector<Vector3>& WindSequence::GetCurrent()
    {
        return windMaps[indexCurrent];
    }

    void WindSequence::AdvanceTimeStep()
    {
        if (looping)
            indexCurrent = (indexCurrent + 1) % windMaps.size();
        else
            indexCurrent = std::min(indexCurrent + 1, windMaps.size() - 1);
    }

    ReadResult WindSequence::parseFile(const std::filesystem::path& path, std::vector<Vector3>& map)
    {
        if (std::filesystem::exists(path))
        {
            try
            {
                std::ifstream infile(path, std::ios_base::binary);

                // header
                int fileVersionMajor, fileVersionMinor;
                infile.read((char*)&fileVersionMajor, sizeof(int));
                infile.read((char*)&fileVersionMinor, sizeof(int));

                // contents
                infile.read((char*)map.data(), sizeof(gaden::Vector3) * map.size());
                infile.close();
                return ReadResult::OK;
            }
            catch (const std::exception& e)
            {
                GADEN_ERROR("Exception when parsing wind file '{}' : '{}'", path, e.what());
            }
        }
        return ReadResult::NO_FILE;
    }
} // namespace gaden