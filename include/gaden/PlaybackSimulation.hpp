#pragma once
#include "Simulation.hpp"
#include "gaden/internal/BufferUtils.hpp"

namespace gaden
{
    class PlaybackSimulation : public Simulation
    {
    public:
        struct Parameters
        {
            size_t startIteration = 0;
            std::filesystem::path simulationDirectory;
        };

    public:
        PlaybackSimulation(Parameters params, EnvironmentConfiguration config, LoopConfig loopConfig);
        void AdvanceTimestep() override;
        const std::vector<Filament>& GetFilaments() const override;

    private:
        void LoadLogfile(BufferReader reader);
        void LoadLogfileVersion1(BufferReader reader);
        void LoadLogfileVersionPre2_6(BufferReader reader);

    private:
        Parameters parameters;
        std::vector<Filament> activeFilaments;
        size_t currentIteration = 0;
        bool firstReading = true;
    };
} // namespace gaden