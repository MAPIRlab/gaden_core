#pragma once
#include "gaden/Environment.hpp"
#include "gaden/internal/WindSequence.hpp"

namespace gaden
{
    class Simulation
    {
    public:
        virtual float SampleConcentration(const Vector3i& indices) const = 0;
        virtual float SampleConcentration(const Vector3& point) const
        {
            return SampleConcentration(environment.coordsToIndices(point));
        };

        virtual Vector3 SampleWind(const Vector3i& indices) const = 0;
        virtual Vector3 SampleWind(const Vector3& point) const
        {
            return SampleWind(environment.coordsToIndices(point));
        }

    public:
        Environment environment;
        WindSequence windSequence;
    };
} // namespace gaden