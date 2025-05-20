#pragma once
#include "Simulation.hpp"
#include "gaden/EnvironmentConfiguration.hpp"

namespace gaden
{
    class RunningSimulation : public Simulation
    {
    public:
        struct Parameters
        {
            GasType gasType = GasType::unknown;
            Vector3 sourcePosition = {0, 0, 0};
            float deltaTime = 0.1;              // seconds
            float temperature = 298.f;          // K
            float pressure = 1.f;               // Atm
            float filament_ppm_center = 20;     //[ppm] Gas concentration at the center of the 3D gaussian (filament)
            float filament_initial_std = 1.5;   //[cm] Sigma of the filament at t=0-> 3DGaussian shape
            float filament_growth_gamma = 10.0; //[cm²/s] Growth ratio of the filament_std
            float filament_noise_std = 0.1;     // STD to add some "variablity" to the filament location
            size_t numFilaments_sec = 100;      // How many filaments to release per second
            size_t expectedNumIterations = 600; // To give initial size to filament vector. If you run the simulator longer than this, there will be a reallocation -- so, bad for performance :_(
        };

    public:
        RunningSimulation(Parameters params, EnvironmentConfiguration config, WindSequence::LoopConfig loopConfig);
        void AdvanceTimestep() override;
        const std::vector<Filament>& GetFilaments() const override;

    private:
        Parameters parameters;

        // ping-pong configuration to avoid deleting filaments from the middle of the vector
        std::vector<Filament> filaments1;
        std::vector<Filament> filaments2;
        std::vector<Filament>* activeFilaments;
    };
} // namespace gaden