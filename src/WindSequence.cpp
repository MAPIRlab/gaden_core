#include "gaden/core/Logging.hpp"
#include "gaden/internal/Utils.hpp"
#include <fstream>
#include <gaden/internal/WindSequence.hpp>

namespace gaden
{

    void WindSequence::Initialize(const std::vector<std::filesystem::path>& files, size_t numCells, LoopConfig loopConf)
    {
        indexCurrent = 0;
        loopConfig = loopConf;
        windMaps.resize(files.size(), std::vector<Vector3>(numCells));
        for (size_t i = 0; i < files.size(); i++)
        {
            const auto& file = files[i];
            if (parseFile(file, windMaps[i]) != ReadResult::OK)
                GADEN_TERMINATE;
        }

        if (loopConfig.loop)
        {
            bool incorrectRange = loopConfig.from > loopConfig.to || !InRange(loopConfig.from, 0, windMaps.size()) || !InRange(loopConfig.to, 0, windMaps.size());
            if (incorrectRange)
            {
                GADEN_WARN("Incorrect loop configuration for wind sequence: {}-{} ({} timesteps exist). Forcing 'loop=false'", loopConfig.from, loopConfig.to, windMaps.size());
                loopConfig.loop = false;
            }
        }
    }

    std::vector<Vector3>& WindSequence::GetCurrent()
    {
        return windMaps[indexCurrent];
    }

    const std::vector<Vector3>& WindSequence::GetCurrent() const
    {
        return windMaps[indexCurrent];
    }

    size_t WindSequence::GetCurrentIndex()
    {
        return indexCurrent;
    }

    void WindSequence::AdvanceTimeStep()
    {
        indexCurrent++;
        if (loopConfig.loop && indexCurrent > loopConfig.to)
            indexCurrent = loopConfig.from;
        else if (indexCurrent > windMaps.size())
            indexCurrent = windMaps.size() - 1;
    }

    void WindSequence::SetCurrentIndex(size_t index)
    {
        if (InRange(index, 0, windMaps.size()))
            indexCurrent = index;
        else
            GADEN_ERROR("Tried to load wind map {} but only {} exist", index, windMaps.size());
    }

    ReadResult WindSequence::parseFile(const std::filesystem::path& path, std::vector<Vector3>& map)
    {
        if (!std::filesystem::exists(path))
            return ReadResult::NO_FILE;

        try
        {
            std::ifstream infile(path, std::ios_base::binary);

            // header
            int fileVersionMajor;
            infile.read((char*)&fileVersionMajor, sizeof(int));

            // the old style of preprocessed-but-not-simulated wind files always started with a 4-byte 999 (which was not really a gaden version, but whatever)
            if (fileVersionMajor == 999)
            {
                GADEN_ERROR("The old style wind files (split into three files with suffixes like _U, _V, _W) are no longer supported. Please convert the files to the new format");
                return ReadResult::READING_FAILED;
            }
            else if (fileVersionMajor == 2)
                parseModernFile(infile, map);
            else
                parseOldFile(infile, map);

            // contents
            infile.close();
            return ReadResult::OK;
        }
        catch (const std::exception& e)
        {
            GADEN_ERROR("Exception when parsing wind file '{}' : '{}'", path.c_str(), e.what());
            return ReadResult::READING_FAILED;
        }
    }

    void WindSequence::parseModernFile(std::ifstream& infile, std::vector<Vector3>& map)
    {
        int fileVersionMinor;
        infile.read((char*)&fileVersionMinor, sizeof(int));
        infile.read((char*)map.data(), sizeof(gaden::Vector3) * map.size());
    }

    void WindSequence::parseOldFile(std::ifstream& infile, std::vector<Vector3>& map)
    {
        // these files have three consecutive arrays of doubles, one for each component
        for (size_t i = 0; i < map.size(); i++)
        {
            double aux;
            infile.read((char*)&aux, sizeof(double));
            map[i].x = aux;
        }
        for (size_t i = 0; i < map.size(); i++)
        {
            double aux;
            infile.read((char*)&aux, sizeof(double));
            map[i].y = aux;
        }
        for (size_t i = 0; i < map.size(); i++)
        {
            double aux;
            infile.read((char*)&aux, sizeof(double));
            map[i].z = aux;
        }
        infile.close();
    }
} // namespace gaden