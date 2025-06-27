#pragma once

#include "GasSource.hpp"
#include "gaden/internal/MathUtils.hpp"

namespace gaden
{
    class LineSource : public GasSource
    {
    public:
        Vector3 Emit() const override
        {
            float t = uniformRandom(0, 1);
            return vmath::lerp(sourcePosition, lineEnd, t);
        }
        const char* Type() const override {return "line";} 

        // sourcePosition is the start of the line segment
        Vector3 lineEnd;
    };
} // namespace gaden