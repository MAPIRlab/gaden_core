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

    RunningSimulation::RunningSimulation(Parameters params, EnvironmentConfiguration const& envConfig)
        : parameters(params), Simulation(envConfig)
    {
        config.windSequence.loopConfig = parameters.windLoop;

        filaments1.reserve(params.expectedNumIterations * params.numFilaments_sec / params.deltaTime);
        filaments2.reserve(params.expectedNumIterations * params.numFilaments_sec / params.deltaTime);
        activeFilaments = &filaments1;
        auxFilamentsVector = &filaments2;

        simulationMetadata.gasType = params.gasType;
        simulationMetadata.sourcePosition = params.sourcePosition;

        // calculate the filament->concentration constants
        //-------------------------------------------------
        constexpr float R = 82.057338; // R is the ideal, or universal, gas constant, equal to the product of the Boltzmann constant and the Avogadro constant [cm³·atm/mol·K] Gas Constant
        simulationMetadata.numMolesAllGasesIncm3 = params.pressure / (R * params.temperature);

        float filament_moles_cm3_center = params.filament_ppm_center / 1e6 * simulationMetadata.numMolesAllGasesIncm3;                          //[moles of target gas / cm³]
        simulationMetadata.totalMolesInFilament = filament_moles_cm3_center * (sqrt(8 * pow(M_PI, 3)) * pow(params.filament_initial_sigma, 3)); // total number of moles in a filament

        if (parameters.saveResults)
            GADEN_INFO("Saving results in directory '{}'", parameters.saveDataDirectory / "result");
    }

    void RunningSimulation::AdvanceTimestep()
    {
        AddFilaments();
        MoveFilaments();

        static float lastSaveTime = -FLT_MAX;
        if (parameters.saveResults && currentTime > lastSaveTime + parameters.saveDeltaTime)
        {
            SaveResults();
            lastSaveTime = currentTime;
        }

        static float lastWindUpdateTime = 0.0;
        if (currentTime > lastWindUpdateTime + parameters.windIterationDeltaTime)
            config.windSequence.AdvanceTimeStep();

        currentTime += parameters.deltaTime;
        currentIteration++;
    }

    const std::vector<Filament>& RunningSimulation::GetFilaments() const
    {
        return *activeFilaments;
    }

    void RunningSimulation::AddFilaments()
    {
        float numFilaments_iteration = parameters.numFilaments_sec * parameters.deltaTime;
        static float accumulator = 0.0; // to handle non-integer values of numFilaments_iteration over multiple iterations

        accumulator += numFilaments_iteration;

        for (size_t i = 0; i < accumulator; i++)
        {
            Vector3 randomOffset = config.environment.description.cellSize * Vector3{uniformRandom(-1, 1), uniformRandom(-1, 1), uniformRandom(-1, 1)};
            Vector3 position = parameters.sourcePosition + randomOffset;
            activeFilaments->emplace_back(position, parameters.filament_initial_sigma);
        }

        accumulator = accumulator - std::floor(accumulator);
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
        size_t gasIndex = static_cast<size_t>(parameters.gasType);

        try
        {
            // Get 3D cell of the filament center
            Vector3i cellIdx = config.environment.coordsToIndices(filament.position);

            // 1. Simulate Advection (Va)
            //    Large scale wind-eddies -> Movement of a filament as a whole by wind
            //------------------------------------------------------------------------
            const gaden::Vector3& windVec = config.windSequence.GetCurrent().at(config.environment.indexFrom3D(cellIdx));
            Vector3 newPosition = filament.position + windVec * parameters.deltaTime;

            // 2. Simulate Gravity & Bouyant Force
            //------------------------------------
            // OLD approach: using accelerations (pure gas)

            // float accel = g * (specific_gravity_air - SpecificGravity[gasIndex]) / SpecificGravity[gasIndex];
            // newpos_z = filament.position_z + 0.5*accel*pow(parameters.deltaTime,2);

            // Approximation from "Terminal Velocity of a Bubble Rise in a Liquid Column", World Academy of Science, Engineering and Technology 28 2007
            constexpr float ro_air = 1.205; //[kg/m³] density of air
            constexpr float mu = 19 * 1e-6; //[kg/s·m] dynamic viscosity of air
            float terminal_buoyancy_velocity = (g * (1 - SpecificGravity[gasIndex]) * ro_air * parameters.filament_ppm_center * 1e-6) / (18 * mu);
            // newpos_z += terminal_buoyancy_velocity*parameters.deltaTime;

            // 3. Add some variability (stochastic process)
            //------------------------------------

            newPosition.x += GaussianRandom(0, parameters.filament_noise_std);
            newPosition.y += GaussianRandom(0, parameters.filament_noise_std);
            newPosition.z += GaussianRandom(0, parameters.filament_noise_std);

            // 4. Check filament location
            //------------------------------------
            Environment::CellState destinationState = StepTowards(filament, newPosition);

            if (destinationState == Environment::CellState::Outlet)
                filament.active = false;

            // 4. Filament growth with time (this affects the posterior estimation of gas concentration at each cell)
            //    Vd (small scale wind eddies) -> Difussion or change of the filament shape (growth with time)
            //    R = sigma of a 3D gaussian -> Increasing sigma with time
            //------------------------------------------------------------------------
            filament.sigma += parameters.filament_growth_gamma / (2 * filament.sigma) * parameters.deltaTime;
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
            if (cellState != Environment::CellState::Free)
            {
                filament.position = previous;
                return cellState;
            }
        }

        // Direct line of sight confirmed!
        return Environment::CellState::Free;
    }

    void RunningSimulation::SaveResults()
    {
        static size_t last_saved_step = 0;

        // check we can create the file
        std::filesystem::path savePath = parameters.saveDataDirectory / "result";
        paths::TryCreateDirectory(savePath);

        // Configure file name for saving the current snapshot
        std::filesystem::path path = fmt::format("{}/iteration_{}", savePath, last_saved_step);

        // write all the data as-is into a buffer, which we will then compress
        static std::vector<uint8_t> rawBuffer(maxBufferSize);
        BufferWriter writer((char*)rawBuffer.data(), rawBuffer.size());

        writer.Write(&gaden::version_major);
        writer.Write(&gaden::version_minor);

        writer.Write(&config.environment.description);

        writer.Write(&simulationMetadata);

        int windIndex = config.windSequence.GetCurrentIndex();
        writer.Write(&windIndex); // index of the wind file (they are stored separately under (results_location)/wind/... )

        for (int i = 0; i < activeFilaments->size(); i++)
        {
            writer.Write(&i);
            writer.Write(&activeFilaments->at(i).position);
            writer.Write(&activeFilaments->at(i).sigma);
        }

        // compression with zlib
        static std::vector<uint8_t> compressedBuffer(maxBufferSize);
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

            saveDataDirectory = path.parent_path();

            // clang-format off
            FromYAML<GasType>   (yaml, "gasType",                   gasType);
            FromYAML<Vector3>   (yaml, "sourcePosition",            sourcePosition);
            FromYAML<float>     (yaml, "deltaTime",                 deltaTime);
            FromYAML<float>     (yaml, "windIterationDeltaTime",    windIterationDeltaTime);
            FromYAML<float>     (yaml, "temperature",               temperature);
            FromYAML<float>     (yaml, "pressure",                  pressure);
            FromYAML<float>     (yaml, "filament_ppm_center",       filament_ppm_center);
            FromYAML<float>     (yaml, "filament_initial_sigma",    filament_initial_sigma);
            FromYAML<float>     (yaml, "filament_growth_gamma",     filament_growth_gamma);
            FromYAML<float>     (yaml, "filament_noise_std",        filament_noise_std);
            FromYAML<float>     (yaml, "numFilaments_sec",          numFilaments_sec);
            FromYAML<size_t>    (yaml, "expectedNumIterations",     expectedNumIterations);
            FromYAML<bool>      (yaml, "saveResults",               saveResults);
            FromYAML<float>     (yaml, "saveDeltaTime",             saveDeltaTime);
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
            emitter << YAML::Flow << YAML::Key << "gasType" << YAML::Value << gasType;
            emitter << YAML::Key << "sourcePosition" << YAML::Value << sourcePosition;
            emitter << YAML::Key << "deltaTime" << YAML::Value << deltaTime;
            emitter << YAML::Key << "windIterationDeltaTime" << YAML::Value << windIterationDeltaTime;
            emitter << YAML::Key << "temperature" << YAML::Value << temperature;
            emitter << YAML::Key << "pressure" << YAML::Value << pressure;
            emitter << YAML::Key << "filament_ppm_center" << YAML::Value << filament_ppm_center;
            emitter << YAML::Key << "filament_initial_sigma" << YAML::Value << filament_initial_sigma;
            emitter << YAML::Key << "filament_growth_gamma" << YAML::Value << filament_growth_gamma;
            emitter << YAML::Key << "filament_noise_std" << YAML::Value << filament_noise_std;
            emitter << YAML::Key << "numFilaments_sec" << YAML::Value << numFilaments_sec;
            emitter << YAML::Key << "expectedNumIterations" << YAML::Value << expectedNumIterations;
            emitter << YAML::Key << "saveResults" << YAML::Value << saveResults;
            emitter << YAML::Key << "saveDeltaTime" << YAML::Value << saveDeltaTime;

            emitter << YAML::Key << "wind_looping";
            emitter << YAML::BeginMap;
            WriteLoopYAML(emitter, windLoop);
            emitter << YAML::EndMap;

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