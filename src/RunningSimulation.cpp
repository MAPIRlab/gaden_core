#include "gaden/RunningSimulation.hpp"

namespace gaden
{

    RunningSimulation::RunningSimulation(Parameters params, EnvironmentConfiguration config, WindSequence::LoopConfig loopConfig)
        : parameters(params)
    {
        environment = config.environment;
        windSequence = config.windSequence;
        windSequence.loopConfig = loopConfig;

        filaments1.reserve(params.expectedNumIterations * params.numFilaments_sec / params.deltaTime);
        filaments2.reserve(params.expectedNumIterations * params.numFilaments_sec / params.deltaTime);

        simulationMetadata.gasType = params.gasType;
        simulationMetadata.sourcePosition = params.sourcePosition;

        // calculate the filament->concentration constants
        //-------------------------------------------------
        constexpr float R = 82.057338; //[cm³·atm/mol·K] Gas Constant
        simulationMetadata.numMolesAllGasesIncm3 = params.pressure / (R * params.temperature);

        float filament_moles_cm3_center = params.filament_ppm_center / 1e6 * simulationMetadata.numMolesAllGasesIncm3;                           //[moles of target gas / cm³]
        simulationMetadata.totalMolesInFilament = filament_moles_cm3_center * (sqrt(8 * pow(3.14159, 3)) * pow(params.filament_initial_std, 3)); // total number of moles in a filament
    }

    void RunningSimulation::AdvanceTimestep()
    {
    }

    const std::vector<Filament>& RunningSimulation::GetFilaments() const
    {
        return *activeFilaments;
    }

} // namespace gaden