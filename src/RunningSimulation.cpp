#include "gaden/RunningSimulation.hpp"
#include "YAML_Conversions.hpp"
#include "gaden/datatypes/GasTypes.hpp"
#include "gaden/internal/MathUtils.hpp"
#include "gaden/internal/Serialization.hpp"
#include <fstream>
#include <gaden/internal/GPUAcceleration.hpp>
#include <gaden/internal/Profiling.hpp>
#include <gaden/internal/compression.hpp>
#include <yaml-cpp/yaml.h>

namespace gaden
{
    static thread_local PrecalculatedGaussian<1000> gaussian; // random values following a gaussian distribution, precomputed for speed

    RunningSimulation::RunningSimulation(Parameters params, std::shared_ptr<EnvironmentConfiguration> const& envConfig)
        : parameters(params), Simulation(envConfig)
    {
        if (parameters.windLoop)
            config->windSequence.loopConfig = *parameters.windLoop;

        // this is a backwards compatibility hack
        // noise used to not consider deltaTime (bad), now it does (good)
        // however, that means that old configuration files will now behave differently (bad)
        // so, to keep things more or less the same (good), we will scale the noise std by the most commonly used delta time -- 1/10th of a second
        parameters.filamentNoise_std *= 10;

        filaments1.reserve(params.expectedNumIterations * params.numFilaments_sec / params.deltaTime);
        filaments2.reserve(params.expectedNumIterations * params.numFilaments_sec / params.deltaTime);
        isActive.reserve(filaments1.capacity());
        activeFilaments = &filaments1;
        auxFilamentsVector = &filaments2;

        simulationMetadata.source = params.source;
        GADEN_VERIFY(simulationMetadata.source->gasType != GasType::unknown, "Gas type is set to 'unknown'! Have you initialized the parameters?");

        // calculate the filament->concentration constants
        //-------------------------------------------------
        simulationMetadata.constants.numMolesAllGasesIncm3 = params.pressure / (R * params.temperature);

        float filament_moles_cm3_center = params.filamentPPMcenter_initial / 1e6 * simulationMetadata.constants.numMolesAllGasesIncm3;                  //[moles of target gas / cm³]
        simulationMetadata.constants.totalMolesInFilament = filament_moles_cm3_center * (sqrt(8 * pow(M_PI, 3)) * pow(params.filamentInitialSigma, 3)); // total number of moles in a filament

        rawBuffer.resize(serialization::defaultBufferSize);
        compressedBuffer.resize(serialization::defaultBufferSize);

        if (config->localAirflowDisturbances.size() != config->environment.numCells())
            config->localAirflowDisturbances.resize(config->environment.numCells(), Vector3(0, 0, 0));

        if (parameters.saveResults)
        {
            GADEN_INFO("Saving results in directory '{}'", parameters.saveDataDirectory);
            std::filesystem::remove_all(parameters.saveDataDirectory); // clear any pre-existing results to avoid mixing two different simulations
            if (!std::filesystem::create_directory(parameters.saveDataDirectory))
                GADEN_ERROR("Could not create directory '{}'", parameters.saveDataDirectory);
        }

        if (parameters.preCalculateConcentrations)
        {
            concentrations.emplace();
            concentrations->resize(envConfig->environment.numCells(), 0.0);
            GADEN_SERIOUS_WARN("\n--------\n"
                               "Using 'preCalculateConcentrations'! This will make the simulation very slow. If you don't actively need this behaviour, it is strongly recommended to turn it off.\n"
                               "--------");

#if GPU_ACCELERATION
            GADEN_INFO_COLOR(fmt::terminal_color::blue, "Using GPU acceleration");
            gpuAcc = std::make_unique<GPUAcceleration>();
            gpuAcc->Setup(config->environment, simulationMetadata.constants);
            gpuCommands.reserve(1e4);
#endif
        }
    }

    RunningSimulation::~RunningSimulation()
    {}

    void RunningSimulation::AdvanceTimestep()
    {
        AddFilaments();
        MoveFilaments();

        if (parameters.saveResults && currentTime > lastSaveTime + parameters.saveDeltaTime)
        {
            if (parameters.preCalculateConcentrations)
#if GPU_ACCELERATION
                UpdateConcentrationsGPU();
#else
                UpdateConcentrationsCPU();
#endif

            SaveResults();
            lastSaveTime = currentTime;
        }

        if (parameters.windLoop && currentTime > lastWindUpdateTime + parameters.windIterationDeltaTime)
        {
            config->windSequence.AdvanceTimeStep();
            lastWindUpdateTime = currentTime;
        }

        currentTime += parameters.deltaTime;
        currentIteration++;
    }

    const std::vector<Filament>& RunningSimulation::GetFilaments() const
    {
        return *activeFilaments;
    }

    Vector3 RunningSimulation::SampleWind(const Vector3i& indices) const
    {
        size_t cellIndex = config->environment.indexFrom3D(indices);
        gaden::Vector3 windVec = config->windSequence.GetCurrent().at(cellIndex) + config->localAirflowDisturbances.at(cellIndex);
        return windVec;
    }

    void RunningSimulation::AddFilaments()
    {
        ZoneScoped;
        float numFilaments_iteration = parameters.numFilaments_sec * parameters.deltaTime;

        releaseAccumulator += numFilaments_iteration;

        for (size_t i = 0; i < std::floor(releaseAccumulator); i++)
        {
            constexpr size_t safetyLimit = 10;
            size_t attempts = 0;

            Vector3 position;
            do
            {
                position = simulationMetadata.source->Emit();
                attempts++;
            } while (!config->environment.IsInBounds(position) && attempts < safetyLimit);

            GADEN_VERIFY(attempts < safetyLimit, "Could not spawn filaments around source position! Is it inside the environment bounds?");

            activeFilaments->emplace_back(position, parameters.filamentInitialSigma);
        }

        releaseAccumulator = releaseAccumulator - std::floor(releaseAccumulator);
    }

    void RunningSimulation::MoveFilaments()
    {
        ZoneScoped;
        isActive.resize(activeFilaments->size());
        std::fill(isActive.begin(), isActive.end(), true);

#pragma omp parallel for
        for (size_t i = 0; i < activeFilaments->size(); i++)
            MoveSingleFilament(i);

        // eliminate filaments that exited the environment and swap the vector pointers
        for (size_t i = 0; i < activeFilaments->size(); i++)
        {
            if (isActive[i])
                auxFilamentsVector->push_back(activeFilaments->at(i));
        }

        activeFilaments->clear();
        std::swap(activeFilaments, auxFilamentsVector);
    }

    void RunningSimulation::MoveSingleFilament(size_t i)
    {
        Filament& filament = activeFilaments->at(i);

        // Estimte filament acceleration due to gravity & Bouyant force (for the given gas_type):
        constexpr float g = 9.8;
        constexpr float specific_gravity_air = 1; //[dimensionless]
        size_t gasIndex = static_cast<size_t>(simulationMetadata.source->gasType);

        try
        {
            // Get 3D cell of the filament center
            Vector3i cellIdx = config->environment.coordsToIndices(filament.position);

            // 1. Simulate Advection (Va)
            //    Large scale wind-eddies -> Movement of a filament as a whole by wind
            //------------------------------------------------------------------------
            gaden::Vector3 windVec = SampleWind(cellIdx);
            Vector3 newPosition = filament.position + windVec * parameters.deltaTime;

            // 2. Simulate Gravity & Bouyant Force
            //------------------------------------
            // OLD approach: using accelerations (pure gas)

            // float accel = g * (specific_gravity_air - SpecificGravity[gasIndex]) / SpecificGravity[gasIndex];
            // newpos_z = filament.position_z + 0.5*accel*pow(parameters.deltaTime,2);

            // Approximation from "Terminal Velocity of a Bubble Rise in a Liquid Column", World Academy of Science, Engineering and Technology 28 2007
            constexpr float ro_air = 1.205; //[kg/m³] density of air
            constexpr float mu = 19 * 1e-6; //[kg/s·m] dynamic viscosity of air
            float terminal_buoyancy_velocity = (g * (1 - SpecificGravity.at(gasIndex)) * ro_air * ConcentrationAtCenter(filament) * 1e-6) / (18 * mu);
            newPosition.z += terminal_buoyancy_velocity * parameters.deltaTime;

            // 3. Add some variability (stochastic process)
            //------------------------------------

            newPosition.x += gaussian.nextValue(0, parameters.filamentNoise_std) * parameters.deltaTime;
            newPosition.y += gaussian.nextValue(0, parameters.filamentNoise_std) * parameters.deltaTime;
            newPosition.z += gaussian.nextValue(0, parameters.filamentNoise_std) * parameters.deltaTime;

            // 4. Check filament location
            //------------------------------------
            Environment::CellState destinationState = StepTowards(filament, newPosition);
            GADEN_ASSERT(config->environment.IsInBounds(filament.position), "Filament is outside environment!");

            if (destinationState == Environment::CellState::Outlet)
                isActive[i] = false;

            // 4. Filament growth with time (this affects the posterior estimation of gas concentration at each cell)
            //    Vd (small scale wind eddies) -> Difussion or change of the filament shape (growth with time)
            //    R = sigma of a 3D gaussian -> Increasing sigma with time
            //------------------------------------------------------------------------
            filament.sigma += parameters.filamentGrowthGamma / (2 * filament.sigma) * parameters.deltaTime;
        }
        catch (std::exception& e)
        {
            GADEN_WARN("Exception Updating Filaments: {}", e.what());
            return;
        }
    }

    // move the filament as much as possible towards the desired final position, stopping if we find an obstacle along the way
    Environment::CellState RunningSimulation::StepTowards(Filament& filament, Vector3 end)
    {
        Vector3i startCell = config->environment.coordsToIndices(filament.position);
        Vector3i endCell = config->environment.coordsToIndices(end);

        if (startCell == endCell)
        {
            filament.position = end;
            return config->environment.at(startCell);
        }

        // Calculate displacement vector
        Vector3 movementDir = end - filament.position;
        float distance = vmath::length(movementDir);
        movementDir = vmath::normalized(movementDir);

        // Traverse path
        int steps = ceil(distance / config->environment.description.cellSize); // Make sure no two iteration steps are separated more than 1 cell
        float increment = distance / steps;

        for (int i = 0; i < steps; i++)
        {
            // Determine point in space to evaluate
            Vector3 previous = filament.position;
            filament.position += movementDir * increment;

            // Check if the cell is occupied
            Environment::CellState cellState = config->environment.at(filament.position);
            if (cellState == Environment::CellState::Obstacle || cellState == Environment::CellState::OutOfBounds)
            {
                Vector3i previousCell = config->environment.coordsToIndices(previous);
                Vector3i currentCell = config->environment.coordsToIndices(filament.position);
                Vector3 normal = previousCell - currentCell;

                filament.position = previous;

                Vector3 remaining = end - filament.position;
                Vector3 rejected = remaining - vmath::project(remaining, normal);

                return StepTowards(filament, filament.position + rejected);
            }
            else if (cellState == Environment::CellState::Outlet)
                return cellState;
        }

        // Direct line of sight confirmed!
        return Environment::CellState::Free;
    }

    void RunningSimulation::UpdateConcentrationsCPU()
    {
#pragma parallel for
        for (size_t i = 0; i < concentrations->size(); i++)
            (*concentrations)[i] = 0;

        for (Filament const& filament : *activeFilaments)
        {
            Vector3i bbMin;
            Vector3i bbMax;
            GetAABB(filament, bbMin, bbMax);

#pragma parallel for collapse(3)
            for (size_t x = bbMin.x; x <= bbMax.x; x++)
                for (size_t y = bbMin.y; y <= bbMax.y; y++)
                    for (size_t z = bbMin.z; z <= bbMax.z; z++)
                    {
                        Vector3i indices{x, y, z};
                        Vector3 samplePoint = config->environment.coordsOfCellCenter(indices);
                        if (CheckLineOfSight(filament.position, samplePoint))
                            concentrations->at(config->environment.indexFrom3D(indices)) += CalculateConcentrationSingleFilament(filament, samplePoint);
                    }
        }
    }

    void RunningSimulation::UpdateConcentrationsGPU()
    {
#if GPU_ACCELERATION

        ZoneScoped;

        for (size_t i = 0; i < activeFilaments->size(); i++)
        {
            Filament const& filament = (*activeFilaments)[i];
            Vector3i bbMin;
            Vector3i bbMax;
            GetAABB(filament, bbMin, bbMax);

            // this is just so we can fill up the vector using multiple threads
            Vector3i extents = (bbMax - bbMin) + Vector3i(1);
            size_t bbSize = extents.x * extents.y * extents.z;
            size_t start = gpuCommands.size();
            gpuCommands.resize(gpuCommands.size() + bbSize);

#pragma omp parallel for collapse(3)
            for (size_t z = bbMin.z; z <= bbMax.z; z++)
                for (size_t y = bbMin.y; y <= bbMax.y; y++)
                    for (size_t x = bbMin.x; x <= bbMax.x; x++)
                    {
                        size_t x_ind = x - bbMin.x;
                        size_t y_ind = y - bbMin.y;
                        size_t z_ind = z - bbMin.z;
                        size_t index = start + x_ind + y_ind * extents.x + z_ind * extents.x * extents.y;
                        gpuCommands.at(index).indices = {x, y, z};
                        gpuCommands.at(index).filament = static_cast<uint32_t>(i);
                    }
        }

        gpuAcc->UpdateConcentrations(gpuCommands, *concentrations, *activeFilaments);
        gpuCommands.clear();
#else
        GADEN_SERIOUS_ERROR("Tried to call UpdateConcentrationsGPU(), but core library was compiled without GPU acceleration support.");
        GADEN_TERMINATE;
#endif
    }

    void RunningSimulation::GetAABB(Filament const& filament, Vector3i& bbMin, Vector3i& bbMax)
    {
        bbMin = config->environment.coordsToIndices(filament.position - filament.sigma * 3 / 100.f);
        bbMax = config->environment.coordsToIndices(filament.position + filament.sigma * 3 / 100.f);

        bbMin.x = std::max(bbMin.x, 0);
        bbMin.y = std::max(bbMin.y, 0);
        bbMin.z = std::max(bbMin.z, 0);

        bbMax.x = std::min(bbMax.x, config->environment.description.dimensions.x - 1);
        bbMax.y = std::min(bbMax.y, config->environment.description.dimensions.y - 1);
        bbMax.z = std::min(bbMax.z, config->environment.description.dimensions.z - 1);
    }

    void RunningSimulation::SaveResults()
    {
        ZoneScoped;
        // check we can create the file
        std::filesystem::path savePath = parameters.saveDataDirectory;

        // Configure file name for saving the current snapshot
        std::filesystem::path path = fmt::format("{}/iteration_{}", savePath, last_saved_step);

        // calculate the size we need the buffer to have
        {
            size_t uncompressedSize;
            if (parameters.preCalculateConcentrations)
                uncompressedSize = concentrations->size() * sizeof(float) + 1000; // 1000 bytes is plenty for the all the environment information
            else
                uncompressedSize = activeFilaments->size() * sizeof(Filament) + 1000;

            if (uncompressedSize > rawBuffer.size())
            {
                size_t newSize = uncompressedSize * 1.5; // try to avoid having to resize all the time
                rawBuffer.resize(newSize);
                GADEN_INFO("Resizing raw buffer to {} bytes", newSize);
                compressedBuffer.resize(newSize);
                GADEN_INFO("Resizing compressed buffer to {} bytes", newSize);
            }
        }

        // write all the data as-is into a buffer, which we will then compress
        serialization::BufferWriter writer((char*)rawBuffer.data(), rawBuffer.size());

        writer.Write(&gaden::versionMajor);
        writer.Write(&gaden::versionMinor);

        writer.Write(&config->environment.description);

        GasSource::SerializeBinary(writer, simulationMetadata.source);
        writer.Write(&simulationMetadata.constants);

        int windIndex = config->windSequence.GetCurrentIndex();
        writer.Write(&windIndex); // index of the wind file (they are stored separately under (results_location)/wind/... )

        if (!parameters.preCalculateConcentrations)
        {
            std::string mode("filaments");
            writer.WriteString(&mode);
            writer.WriteVector(activeFilaments);
        }
        else
        {
            ZoneScopedN("WriteConcentrations");
            std::string mode("concentrations");
            writer.WriteString(&mode);
            writer.WriteVector(&(*concentrations));
        }

        // decide which compression algorithm to use. ZLIB is faster for small files, LIBBSC is faster for large files
        serialization::CompressionMode compressionMode;
        size_t uncompressedSize = writer.currentOffset();
        if (uncompressedSize > 5e6)
            compressionMode = serialization::CompressionMode::LIBBSC;
        else
            compressionMode = serialization::CompressionMode::ZLIB;

        // compression with zlib
        zlib::uLongf destLength = compressedBuffer.size();
        {
            ZoneScopedN("Compress");
            if (compressionMode == serialization::CompressionMode::LIBBSC)
                destLength = LibBSC::Compress(rawBuffer.data(), writer.currentOffset(), compressedBuffer.data());
            else if (compressionMode == serialization::CompressionMode::ZLIB)
                zlib::compress2(compressedBuffer.data(), &destLength, rawBuffer.data(), writer.currentOffset(), 1);
            else if (compressionMode == serialization::CompressionMode::UNCOMPRESSED)
                memcpy(compressedBuffer.data(), rawBuffer.data(), writer.currentOffset());
        }

        // write to disk
        {
            ZoneScopedN("WriteToDisk");

            std::ofstream results_file(path);
            // file header
            results_file.write(serialization::resultIdentifier, sizeof(serialization::resultIdentifier));
            results_file.write((char*)&compressionMode, sizeof(compressionMode));
            results_file.write((char*)&uncompressedSize, sizeof(uncompressedSize));

            results_file.write((char*)compressedBuffer.data(), destLength);
            results_file.close();

            // when profiling as sudo, files are created as owned by root, which means you then can't delete them as a normal user
            std::filesystem::permissions(
                path,
                std::filesystem::perms::owner_all | std::filesystem::perms::group_all,
                std::filesystem::perm_options::add);
        }
        last_saved_step++;
    }

    void RunningSimulation::Parameters::ReadFromYAML(std::filesystem::path const& path)
    {
        try
        {
            const YAML::Node yaml = YAML::LoadFile(path);

            saveDataDirectory = path.parent_path() / "result";

            if (!yaml["source"])
            {
                source = std::make_shared<PointSource>();
                GADEN_ERROR("Yaml does not include a source object! Creating a default point source at (0,0,0)");
            }
            else
                source = GasSource::ParseYAML(yaml["source"]);

            // clang-format off
            FromYAML<float>     (yaml, "deltaTime",                 deltaTime);
            FromYAML<float>     (yaml, "windIterationDeltaTime",    windIterationDeltaTime);
            FromYAML<float>     (yaml, "temperature",               temperature);
            FromYAML<float>     (yaml, "pressure",                  pressure);
            FromYAML<float>     (yaml, "filamentPPMcenter",         filamentPPMcenter_initial);
            FromYAML<float>     (yaml, "filamentInitialSigma",      filamentInitialSigma);
            FromYAML<float>     (yaml, "filamentGrowthGamma",       filamentGrowthGamma);
            FromYAML<float>     (yaml, "filamentNoise_std",         filamentNoise_std);
            FromYAML<float>     (yaml, "numFilaments_sec",          numFilaments_sec);
            FromYAML<size_t>    (yaml, "expectedNumIterations",     expectedNumIterations);
            FromYAML<bool>      (yaml, "saveResults",               saveResults);
            FromYAML<float>     (yaml, "saveDeltaTime",             saveDeltaTime);
            FromYAML<bool>      (yaml, "preCalculateConcentrations",preCalculateConcentrations);
            // clang-format on

            if (YAML::Node wind_yaml = yaml["windLooping"])
                windLoop = ParseLoopYAML(wind_yaml);
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Exception while reading simulation params from file '{}': {}", path, e.what());
        }
    }

    bool RunningSimulation::Parameters::WriteToYAML(std::filesystem::path const& path)
    {
        try
        {
            YAML::Emitter emitter;
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "source" << YAML::Value;
            GasSource::WriteYAML(emitter, source);

            // clang-format off
            emitter << YAML::Key << "deltaTime"                 << YAML::Value << deltaTime;
            emitter << YAML::Key << "windIterationDeltaTime"    << YAML::Value << windIterationDeltaTime;
            emitter << YAML::Key << "temperature"               << YAML::Value << temperature;
            emitter << YAML::Key << "pressure"                  << YAML::Value << pressure;
            emitter << YAML::Key << "filamentPPMcenter"         << YAML::Value << filamentPPMcenter_initial;
            emitter << YAML::Key << "filamentInitialSigma"      << YAML::Value << filamentInitialSigma;
            emitter << YAML::Key << "filamentGrowthGamma"       << YAML::Value << filamentGrowthGamma;
            emitter << YAML::Key << "filamentNoise_std"         << YAML::Value << filamentNoise_std;
            emitter << YAML::Key << "numFilaments_sec"          << YAML::Value << numFilaments_sec;
            emitter << YAML::Key << "expectedNumIterations"     << YAML::Value << expectedNumIterations;
            emitter << YAML::Key << "saveResults"               << YAML::Value << saveResults;
            emitter << YAML::Key << "saveDeltaTime"             << YAML::Value << saveDeltaTime;
            emitter << YAML::Key << "preCalculateConcentrations"<< YAML::Value << preCalculateConcentrations;
            // clang-format on

            if (windLoop)
            {
                emitter << YAML::Key << "windLooping";
                WriteLoopYAML(emitter, *windLoop);
            }

            std::ofstream file(path);
            file << emitter.c_str();
            file.close();
            GADEN_INFO("Wrote configuration to '{}'", path);
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Failed to write simulation parameters to '{}'", path);
            return false;
        }

        return true;
    }

} // namespace gaden
