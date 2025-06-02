#include "gaden/internal/BufferUtils.hpp"
#include <fstream>
#include <gaden/PlaybackSimulation.hpp>
#include <gaden/internal/compression.hpp>

namespace gaden
{

    PlaybackSimulation::PlaybackSimulation(Parameters params, EnvironmentConfiguration const& config, LoopConfig loop)
        : Simulation(config), parameters(params), loopConfig(loop)
    {
        if (loopConfig.from > loopConfig.to)
        {
            GADEN_ERROR("Incorrect loop configuration {}-{}. Setting loop to false", loopConfig.from, loopConfig.to);
            loopConfig.loop = false;
        }
    }

    void PlaybackSimulation::AdvanceTimestep()
    {
        std::string filename = fmt::format("{}/iteration_{}", parameters.simulationDirectory.c_str(), currentIteration);
        if (!std::filesystem::exists(filename))
        {
            GADEN_ERROR("File '{}' does not exist!", filename.c_str());
            currentIteration++;
            return;
        }

        // read the file
        std::ifstream infile(filename, std::ios_base::binary | std::ios_base::ate);
        size_t streamSize = infile.tellg();
        infile.seekg(0, std::ios_base::beg);
        static std::vector<uint8_t> compressedBuffer(maxBufferSize);
        infile.read((char*)compressedBuffer.data(), streamSize);
        infile.close();

        // decompress the contents
        static std::vector<uint8_t> rawBuffer(maxBufferSize);
        zlib::uLongf bufferSize = rawBuffer.size();
        zlib::uncompress(rawBuffer.data(), &bufferSize, compressedBuffer.data(), compressedBuffer.size());

        BufferReader reader((char*)rawBuffer.data(), bufferSize);

        // check the version of gaden used to generate the file
        reader.Read(&config.environment.versionMajor);
        if (config.environment.versionMajor == 1)
        {
            config.environment.versionMinor = 0; // pre 2.0 files had no minor version
            LoadLogfileVersion1(reader);
        }
        else if (config.environment.versionMajor == 2)
        {
            reader.Read(&config.environment.versionMinor, sizeof(int));
            if (config.environment.versionMinor <= 5)
                LoadLogfileVersionPre2_6(reader);
            else
                LoadLogfile(reader);
        }
        else
        {
            reader.Read(&config.environment.versionMinor, sizeof(int));
            LoadLogfile(reader);
        }
        currentIteration++;

        if (loopConfig.loop && currentIteration > loopConfig.to)
            currentIteration = loopConfig.from;
    }

    const std::vector<Filament>& PlaybackSimulation::GetFilaments() const
    {
        return activeFilaments;
    }

    void PlaybackSimulation::LoadLogfile(BufferReader reader)
    {
        reader.Read(&config.environment.description);
        reader.Read(&simulationMetadata);

        int windIndex;
        reader.Read(&windIndex, sizeof(int));
        config.windSequence.SetCurrentIndex(windIndex);

        activeFilaments.clear();
        int filament_index;
        float x, y, z, stdDev;
        while (!reader.Ended())
        {
            reader.Read(&filament_index, sizeof(int));
            reader.Read(&x, sizeof(float));
            reader.Read(&y, sizeof(float));
            reader.Read(&z, sizeof(float));
            reader.Read(&stdDev, sizeof(float));

            activeFilaments.emplace_back(x, y, z, stdDev);
        }
    }

} // namespace gaden

// backwards compatibility file parsing
//-------------------------------------
namespace gaden
{
    void PlaybackSimulation::LoadLogfileVersion1(BufferReader reader)
    {
        static bool warned = false;
        if (!warned)
        {
            GADEN_WARN(
                "You are reading a log file that was generated with an old version of gaden. While it should work correctly, if you experience any bugs, "
                "this is likely the cause. You can just re-run the filament simulator node to generate an updated version of the simulation");
            warned = true;
        }

        // coordinates were initially written as doubles, but we want to read them as floats now, so we need a buffer
        double bufferDoubles[5];
        reader.Read(&bufferDoubles, 3 * sizeof(double));
        config.environment.description.minCoord.x = bufferDoubles[0];
        config.environment.description.minCoord.y = bufferDoubles[1];
        config.environment.description.minCoord.z = bufferDoubles[2];

        reader.Read(&bufferDoubles, 3 * sizeof(double));
        config.environment.description.maxCoord.x = bufferDoubles[0];
        config.environment.description.maxCoord.y = bufferDoubles[1];
        config.environment.description.maxCoord.z = bufferDoubles[2];

        reader.Read(&config.environment.description.dimensions.x, sizeof(int));
        reader.Read(&config.environment.description.dimensions.y, sizeof(int));
        reader.Read(&config.environment.description.dimensions.z, sizeof(int));

        reader.Read(&bufferDoubles, 3 * sizeof(double));
        config.environment.description.cellSize = bufferDoubles[0];

        reader.Read(&bufferDoubles, 3 * sizeof(double)); // ground truth source position, we can ignore it

        int gas_type_index;
        reader.Read(&gas_type_index, sizeof(int));
        simulationMetadata.gasType = static_cast<GasType>(gas_type_index);

        reader.Read(bufferDoubles, sizeof(double));
        simulationMetadata.totalMolesInFilament = bufferDoubles[0];
        reader.Read(bufferDoubles, sizeof(double));
        simulationMetadata.numMolesAllGasesIncm3 = bufferDoubles[0];

        int windIndex;
        reader.Read(&windIndex, sizeof(int));
        config.windSequence.SetCurrentIndex(windIndex);

        activeFilaments.clear();
        int filament_index;
        double x, y, z, stdDev;
        while (!reader.Ended())
        {
            reader.Read(&filament_index, sizeof(int));
            reader.Read(&x, sizeof(double));
            reader.Read(&y, sizeof(double));
            reader.Read(&z, sizeof(double));
            reader.Read(&stdDev, sizeof(double));

            activeFilaments.emplace_back(x, y, z, stdDev);
        }
    }

    void PlaybackSimulation::LoadLogfileVersionPre2_6(BufferReader reader)
    {
        reader.Read(&config.environment.description, sizeof(config.environment.description));
        gaden::Vector3 source_position;
        reader.Read(&source_position, sizeof(gaden::Vector3));

        int gas_type_index;
        reader.Read(&gas_type_index, sizeof(int));
        simulationMetadata.gasType = static_cast<GasType>(gas_type_index);

        double aux;
        reader.Read(&aux, sizeof(double));
        simulationMetadata.totalMolesInFilament = aux;
        reader.Read(&aux, sizeof(double));
        simulationMetadata.numMolesAllGasesIncm3 = aux;

        int windIndex;
        reader.Read(&windIndex, sizeof(int));
        config.windSequence.SetCurrentIndex(windIndex);

        activeFilaments.clear();
        int filament_index;
        double x, y, z, stdDev;
        while (!reader.Ended())
        {
            reader.Read(&filament_index, sizeof(int));
            reader.Read(&x, sizeof(double));
            reader.Read(&y, sizeof(double));
            reader.Read(&z, sizeof(double));
            reader.Read(&stdDev, sizeof(double));

            activeFilaments.emplace_back(x, y, z, stdDev);
        }
    }
} // namespace gaden