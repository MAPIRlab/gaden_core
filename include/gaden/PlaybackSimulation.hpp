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
            std::filesystem::path resultsDirectory;
        };

    public:
        PlaybackSimulation() = delete;
        PlaybackSimulation(Parameters params, EnvironmentConfiguration const& config, LoopConfig loop);
        void AdvanceTimestep() override;
        const std::vector<Filament>& GetFilaments() const override;

    private:
        void LoadLogfile(BufferReader reader);
        void LoadLogfileVersion1(BufferReader reader);
        void LoadLogfileVersionPre2_6(BufferReader reader);

    private:
        Parameters parameters;
        LoopConfig loopConfig;
        std::vector<Filament> activeFilaments;
        size_t currentIteration = 0;
        bool firstReading = true;

        std::vector<uint8_t> compressedBuffer;
        std::vector<uint8_t> rawBuffer;

    };
} // namespace gaden