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
            float windIterationDeltaTime = 1.0; // seconds
            float temperature = 298.f;          // K
            float pressure = 1.f;               // Atm
            float filament_ppm_center = 20;     //[ppm] Gas concentration at the center of the 3D gaussian (filament)
            float filament_initial_sigma = 1.5; //[cm] Sigma of the filament at t=0-> 3DGaussian shape
            float filament_growth_gamma = 10.0; //[cm²/s] Growth ratio of the filament_std
            float filament_noise_std = 0.1;     // STD to add some "variablity" to the filament location
            float numFilaments_sec = 100;       // How many filaments to release per second
            size_t expectedNumIterations = 600; // To give initial size to filament vector. If you run the simulator longer than this, there will be a reallocation -- so, bad for performance :_(

            // you can query the simulation as it runs, or store the state of the gas dispersion to disk and play it back later
            bool saveResults = false;
            float saveDeltaTime = 0.5;
            std::string simulationID = "sim1"; // used to name the directory in which the results are stored
        };

    public:
        RunningSimulation(Parameters params, EnvironmentConfiguration config, LoopConfig loopConfig);
        void AdvanceTimestep() override;
        const std::vector<Filament>& GetFilaments() const override;

    private:
        void AddFilaments();
        void MoveFilaments();
        void MoveSingleFilament(Filament& filament);
        Environment::CellState StepTowards(Filament& filament, Vector3 end);

        void SaveResults();

    private:
        Parameters parameters;

        // ping-pong configuration to avoid deleting filaments from the middle of the vector
        // for any given iteration, only one of the vectors actually contains the filaments (pointed at by activeFilaments)
        // when iterating over all the filaments to update their positions, we copy all the ones that survive to the other vector
        // then, we clear activeFilaments and we swap the pointers
        std::vector<Filament> filaments1;
        std::vector<Filament> filaments2;
        std::vector<Filament>* activeFilaments;
        std::vector<Filament>* auxFilamentsVector;

        float currentTime = 0.0;
        size_t currentIteration = 0;
    };
} // namespace gaden