#pragma once
#include "gaden/EnvironmentConfiguration.hpp"
#include "gaden/datatypes/Filament.hpp"
#include "gaden/datatypes/SimulationMetadata.hpp"

namespace gaden
{
    // abstract base class for simulations
    // will be instantiated as either a pre-computed or a real-time simulation
    class Simulation
    {
    public:
        Simulation(const EnvironmentConfiguration& configuration)
            : config(configuration)
        {}
        
        virtual void AdvanceTimestep() = 0;
        float SampleConcentration(const Vector3& point) const;

        virtual Vector3 SampleWind(const Vector3i& indices) const;
        Vector3 SampleWind(const Vector3& point) const;

        virtual const std::vector<Filament>& GetFilaments() const = 0;

    protected:
        bool CheckLineOfSight(Vector3 start, Vector3 end) const;
        float CalculateConcentrationSingleFilament(const Filament& filament, const Vector3& samplePoint) const;

    public:
        EnvironmentConfiguration config;
        SimulationMetadata simulationMetadata;
    };
} // namespace gaden