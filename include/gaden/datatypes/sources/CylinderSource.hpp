#pragma once

#include "gaden/datatypes/sources/GasSource.hpp"
#include "gaden/internal/MathUtils.hpp"
namespace gaden
{
    class CylinderSource : public GasSource
    {
    public:
        Vector3 Emit() const override
        {
            Vector2 pointInCircle;

            do
            {
                pointInCircle = {uniformRandom(-radius, radius),
                                 uniformRandom(-radius, radius)};
            } while (vmath::sqrlength(pointInCircle) > radius * radius);

            float l = uniformRandom(-height * 0.5, height * 0.5);

            return sourcePosition + Vector3(pointInCircle.x, pointInCircle.y, l);
        }

        const char* Type() const override
        {
            return "cylinder";
        }

        float radius = 1;
        float height = 0.5;
    };
} // namespace gaden