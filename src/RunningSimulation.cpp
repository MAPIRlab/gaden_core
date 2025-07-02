#include "gaden/RunningSimulation.hpp"
#include "YAML_Conversions.hpp"
#include "gaden/datatypes/GasTypes.hpp"
#include "gaden/internal/BufferUtils.hpp"
#include "gaden/internal/MathUtils.hpp"
#include "gaden/internal/PathUtils.hpp"
#include <fstream>
#include <gaden/internal/compression.hpp>
#include <yaml-cpp/yaml.h>

namespace gaden
{
    static thread_local PrecalculatedGaussian<1000> gaussian; // random values following a gaussian distribution, precomputed for speed

    RunningSimulation::RunningSimulation(Parameters params, EnvironmentConfiguration const& envConfig)
        : parameters(params), Simulation(envConfig)
    {
        config.windSequence.loopConfig = parameters.windLoop;

        // this is a backwards compatibility hack
        // noise used to not consider deltaTime (bad), now it does (good)
        // however, that means that old configuration files will now behave differently (bad)
        // so, to keep things more or less the same (good), we will scale the noise std by the most commonly used delta time -- 1/10th of a second
        parameters.filamentNoise_std *= 10;

        filaments1.reserve(params.expectedNumIterations * params.numFilaments_sec / params.deltaTime);
        filaments2.reserve(params.expectedNumIterations * params.numFilaments_sec / params.deltaTime);
        activeFilaments = &filaments1;
        auxFilamentsVector = &filaments2;

        simulationMetadata.source = params.source;
        GADEN_VERIFY(simulationMetadata.source->gasType != GasType::unknown, "Gas type is set to 'unknown'! Have you initialized the parameters?");

        // calculate the filament->concentration constants
        //-------------------------------------------------
        simulationMetadata.constants.numMolesAllGasesIncm3 = params.pressure / (R * params.temperature);

        float filament_moles_cm3_center = params.filamentPPMcenter_initial / 1e6 * simulationMetadata.constants.numMolesAllGasesIncm3;                  //[moles of target gas / cm³]
        simulationMetadata.constants.totalMolesInFilament = filament_moles_cm3_center * (sqrt(8 * pow(M_PI, 3)) * pow(params.filamentInitialSigma, 3)); // total number of moles in a filament

        rawBuffer.resize(maxBufferSize);
        compressedBuffer.resize(maxBufferSize);
        localAirflowDisturbances.resize(config.environment.numCells(), Vector3(0, 0, 0));

        paths::TryCreateDirectory(parameters.saveDataDirectory);
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
            concentrations->resize(envConfig.environment.numCells(), 0.0);
            GADEN_SERIOUS_WARN("\n--------\n"
                               "Using 'preCalculateConcentrations'! This will make the simulation very slow. If you don't actively need this behaviour, it is strongly recommended to turn it off.\n"
                               "--------");
        }
    }

    void RunningSimulation::AdvanceTimestep()
    {
        AddFilaments();
        MoveFilaments();

        if (parameters.preCalculateConcentrations)
            UpdateConcentrations();

        if (parameters.saveResults && currentTime > lastSaveTime + parameters.saveDeltaTime)
        {
            SaveResults();
            lastSaveTime = currentTime;
        }

        if (currentTime > lastWindUpdateTime + parameters.windIterationDeltaTime)
        {
            config.windSequence.AdvanceTimeStep();
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
        size_t cellIndex = config.environment.indexFrom3D(indices);
        gaden::Vector3 windVec = config.windSequence.GetCurrent().at(cellIndex) + localAirflowDisturbances.at(cellIndex);
        return windVec;
    }

    void RunningSimulation::AddFilaments()
    {
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
            } while (!config.environment.IsInBounds(position) && attempts < safetyLimit);

            GADEN_VERIFY(attempts < safetyLimit, "Could not spawn filaments around source position! Is it inside the environment bounds?");

            activeFilaments->emplace_back(position, parameters.filamentInitialSigma);
        }

        releaseAccumulator = releaseAccumulator - std::floor(releaseAccumulator);
    }

    void RunningSimulation::MoveFilaments()
    {
#pragma omp parallel for
        for (size_t i = 0; i < activeFilaments->size(); i++)
            MoveSingleFilament(activeFilaments->at(i));

        // eliminate filaments that exited the environment and swap the vector pointers
        for (size_t i = 0; i < activeFilaments->size(); i++)
        {
            Filament& filament = activeFilaments->at(i);
            if (filament.active)
                auxFilamentsVector->push_back(filament);
        }

        activeFilaments->clear();
        std::swap(activeFilaments, auxFilamentsVector);
    }

    void RunningSimulation::MoveSingleFilament(Filament& filament)
    {
        // Estimte filament acceleration due to gravity & Bouyant force (for the given gas_type):
        constexpr float g = 9.8;
        constexpr float specific_gravity_air = 1; //[dimensionless]
        size_t gasIndex = static_cast<size_t>(simulationMetadata.source->gasType);

        try
        {
            // Get 3D cell of the filament center
            Vector3i cellIdx = config.environment.coordsToIndices(filament.position);

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

            if (destinationState == Environment::CellState::Outlet)
                filament.active = false;

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
        Vector3i startCell = config.environment.coordsToIndices(filament.position);
        Vector3i endCell = config.environment.coordsToIndices(end);

        if (startCell == endCell)
        {
            filament.position = end;
            return config.environment.at(startCell);
        }

        // Calculate displacement vector
        Vector3 movementDir = end - filament.position;
        float distance = vmath::length(movementDir);
        movementDir = vmath::normalized(movementDir);

        // Traverse path
        int steps = ceil(distance / config.environment.description.cellSize); // Make sure no two iteration steps are separated more than 1 cell
        float increment = distance / steps;

        for (int i = 0; i < steps; i++)
        {
            // Determine point in space to evaluate
            Vector3 previous = filament.position;
            filament.position += movementDir * increment;

            // Check if the cell is occupied
            Environment::CellState cellState = config.environment.at(filament.position);
            if (cellState == Environment::CellState::Obstacle || cellState == Environment::CellState::OutOfBounds)
            {
                Vector3i previousCell = config.environment.coordsToIndices(previous);
                Vector3i currentCell = config.environment.coordsToIndices(filament.position);
                Vector3 normal = previousCell - currentCell;

                filament.position = previous;

                Vector3 remaining = end-filament.position;
                Vector3 rejected = remaining - vmath::project(remaining, normal);

                return StepTowards(filament, filament.position + rejected);
            }
            else if(cellState == Environment::CellState::Outlet)
                return cellState;
        }

        // Direct line of sight confirmed!
        return Environment::CellState::Free;
    }

    void RunningSimulation::UpdateConcentrations()
    {
#pragma parallel for
        for (size_t i = 0; i < concentrations->size(); i++)
        {
            if (config.environment.cells[i] == Environment::CellState::Free)
                (*concentrations)[i] += CalculateConcentration(config.environment.coordsOfCellCenter(config.environment.indicesFrom1D(i)));
        }
    }

    void RunningSimulation::SaveResults()
    {
        // check we can create the file
        std::filesystem::path savePath = parameters.saveDataDirectory;

        // Configure file name for saving the current snapshot
        std::filesystem::path path = fmt::format("{}/iteration_{}", savePath, last_saved_step);

        // write all the data as-is into a buffer, which we will then compress
        BufferWriter writer((char*)rawBuffer.data(), rawBuffer.size());

        writer.Write(&gaden::versionMajor);
        writer.Write(&gaden::versionMinor);

        writer.Write(&config.environment.description);

        GasSource::SerializeBinary(writer, simulationMetadata.source);
        writer.Write(&simulationMetadata.constants);

        int windIndex = config.windSequence.GetCurrentIndex();
        writer.Write(&windIndex); // index of the wind file (they are stored separately under (results_location)/wind/... )

        if (!parameters.preCalculateConcentrations)
        {
            std::string mode("filaments");
            writer.Write(&mode);
            writer.Write(activeFilaments);
        }
        else
        {
            std::string mode("concentrations");
            writer.Write(&mode);
            writer.Write(&(*concentrations));
        }

        // compression with zlib
        zlib::uLongf destLength = compressedBuffer.size();
        zlib::compress2(compressedBuffer.data(), &destLength, rawBuffer.data(), writer.currentOffset(), Z_DEFAULT_COMPRESSION);

        // write to disk
        std::ofstream results_file(path);
        results_file.write((char*)compressedBuffer.data(), destLength);
        results_file.close();
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

            if (YAML::Node wind_yaml = yaml["wind_looping"])
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

            emitter << YAML::Key << "windLooping";
            WriteLoopYAML(emitter, windLoop);

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