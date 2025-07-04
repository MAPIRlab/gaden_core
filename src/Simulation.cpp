#include "gaden/core/Logging.hpp"
#include <gaden/Simulation.hpp>

namespace gaden
{
    bool Simulation::CheckLineOfSight(Vector3 start, Vector3 end) const
    {
        // Check whether one of the points is outside the valid environment or is not free
        if (config.environment.at(start) != Environment::CellState::Free || config.environment.at(end) != Environment::CellState::Free)
            return false;

        // Calculate displacement vector
        Vector3 vector = end - start;
        float distance = vmath::length(vector);
        vector = vector / distance;

        // Traverse path
        int steps = distance / config.environment.description.cellSize; // Make sure no two iteration steps are separated more than 1 cell
        float increment = distance / steps;

        for (int i = 1; i < steps; i++)
        {
            // Determine point in space to evaluate
            Vector3 position = start + vector * (increment * i);

            // Check if the cell is occupied
            if (config.environment.at(position) != Environment::CellState::Free)
                return false;
        }

        // Direct line of sight confirmed!
        return true;
    }

    float Simulation::CalculateConcentration(const Vector3& samplePoint) const
    {
        float gas_conc = 0;
        const auto& activeFilaments = GetFilaments();
        for (auto it = activeFilaments.begin(); it != activeFilaments.end(); it++)
        {
            const Filament& fil = *it;
            float distanceSqr = vmath::sqrlength(fil.position - samplePoint);

            float limitDistance = fil.sigma * 3 / 100.f; // arbitrary cutoff point at 3 sigma
            if (distanceSqr < limitDistance * limitDistance && CheckLineOfSight(samplePoint, fil.position))
                gas_conc += CalculateConcentrationSingleFilament(fil, samplePoint);
        }

        return gas_conc;
    }

    float Simulation::CalculateConcentrationSingleFilament(const Filament& filament, const Vector3& samplePoint) const
    {
        // calculate how much gas concentration does one filament contribute to the queried location
        float sigma = filament.sigma;
        float distance_cm = 100 * vmath::length(filament.position - samplePoint);

        float ppm = ConcentrationAtCenter(filament) *
                    exp(-(distance_cm * distance_cm) / (2 * sigma * sigma));
        return ppm;
    }

    float Simulation::ConcentrationAtCenter(Filament const& filament) const
    {
        constexpr float pi_cubed = M_PI * M_PI * M_PI;

        float numMolesTarget_cm3 = simulationMetadata.constants.totalMolesInFilament //
                                   / (sqrt(8 * pi_cubed) * filament.sigma * filament.sigma * filament.sigma);

        return 1e6 * numMolesTarget_cm3 / simulationMetadata.constants.numMolesAllGasesIncm3; // express in ppm
    }

    float Simulation::SampleConcentration(const Vector3& samplePoint) const
    {
        if (!config.environment.IsInBounds(samplePoint))
        {
            GADEN_ERROR("Requested gas concentration at a point outside the environment {}. Are you using the correct coordinates?", samplePoint);
            return 0;
        }

        if (concentrations)
            return concentrations->at(config.environment.indexFrom3D(config.environment.coordsToIndices(samplePoint)));
        else
            return CalculateConcentration(samplePoint);
    }

    Vector3 Simulation::SampleWind(const Vector3i& indices) const
    {
        return config.windSequence.GetCurrent().at(config.environment.indexFrom3D(indices));
    }

    Vector3 Simulation::SampleWind(const Vector3& point) const
    {
        return SampleWind(config.environment.coordsToIndices(point));
    }
} // namespace gaden