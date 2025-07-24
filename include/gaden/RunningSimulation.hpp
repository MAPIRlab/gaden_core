#pragma once
#include "Simulation.hpp"
#include "gaden/EnvironmentConfiguration.hpp"
#include "gaden/datatypes/sources/PointSource.hpp"

namespace gaden
{
    class RunningSimulation : public Simulation
    {
    public:
        struct Parameters
        {
            std::shared_ptr<GasSource> source = std::make_shared<PointSource>();

            float deltaTime = 0.1;                // seconds
            float windIterationDeltaTime = 1.0;   // seconds
            float temperature = 298.f;            // K
            float pressure = 1.f;                 // Atm
            float filamentPPMcenter_initial = 20; //[ppm] Gas concentration at the center of the 3D gaussian (filament)
            float filamentInitialSigma = 10.0;    //[cm] Sigma of the filament at t=0-> 3DGaussian shape
            float filamentGrowthGamma = 10.0;     //[cm²/s] Growth ratio of the filament_std
            float filamentNoise_std = 0.02;       // STD to add some "variablity" to the filament location
            float numFilaments_sec = 10;          // How many filaments to release per second
            size_t expectedNumIterations = 600;   // To give initial size to filament vector. If you run the simulator longer than this, there will be a reallocation -- so, bad for performance :_(

            std::optional<LoopConfig> windLoop; // if this is not null, the simulation advances the wind sequence as time passes

            // you can query the simulation as it runs, or store the state of the gas dispersion to disk and play it back later
            bool saveResults = false;
            float saveDeltaTime = 0.5;
            bool preCalculateConcentrations = false; // produce full concentration maps instead of serializing the filament positions. NOT RECOMMENDED!
                                                     // it is *way* slower and produces *much* larger files, but could be useful for some applications
            std::filesystem::path saveDataDirectory;

            void ReadFromYAML(std::filesystem::path const& path);
            bool WriteToYAML(std::filesystem::path const& path);
        };

    public:
        RunningSimulation(Parameters params, std::shared_ptr<EnvironmentConfiguration> const& envConfig);
        ~RunningSimulation(); // we need to declare a destructor because of the unique_ptr to incomplete type (GPUAcc)
        void AdvanceTimestep() override;
        const std::vector<Filament>& GetFilaments() const override;
        float GetCurrentTime() { return currentTime; }
        const Parameters& GetParameters() { return parameters; }

        Vector3 SampleWind(const Vector3i& indices) const override;

    private:
        void AddFilaments();
        void MoveFilaments();
        void MoveSingleFilament(size_t i);
        Environment::CellState StepTowards(Filament& filament, Vector3 end);
        void SaveResults();

        // Only used in preCalculateConcentrations mode
        void UpdateConcentrationsCPU();
        void UpdateConcentrationsGPU();
        void GetAABB(Filament const& filament, Vector3i& min, Vector3i& max);

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
        std::vector<uint8_t> isActive;

        float currentTime = 0.0;
        size_t currentIteration = 0;

        float lastSaveTime = -FLT_MAX;
        float lastWindUpdateTime = 0.0;

        float releaseAccumulator = 0.0; // to handle non-integer values of numFilaments_iteration over multiple iterations

        size_t last_saved_step = 0;
        std::vector<uint8_t> rawBuffer;
        std::vector<uint8_t> compressedBuffer;

        // these can only be used if GPU_ACCELERATION=1
        std::unique_ptr<class GPUAcceleration> gpuAcc;
        std::vector<class ComputeConcentrationCommand> gpuCommands;
    };
} // namespace gaden