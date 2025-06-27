#pragma once
#include "GasSource.hpp"
#include "gaden/internal/MathUtils.hpp"

namespace gaden
{
    class SphereSource : public GasSource
    {
    public:
        virtual Vector3 Emit() const override
        {
            // rejection sampling
            // looks funky, but it is generally faster than generating the points in spherical coordinates and converting to cartesian
            while (true)
            {
                Vector3 offset(uniformRandom(-radius, radius),
                               uniformRandom(-radius, radius),
                               uniformRandom(-radius, radius));
                float sqrDistance = vmath::sqrlength(offset);
                if (sqrDistance <= squareRadius)
                    return sourcePosition + offset;
            }
        }
        const char* Type() const override {return "sphere";} 

        void SetRadius(float r)
        {
            radius = r;
            squareRadius = r * r;
        }

        float GetRadius() { return radius; }

    private:
        float radius;
        float squareRadius;
    };
} // namespace gaden