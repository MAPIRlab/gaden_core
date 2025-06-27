#pragma once

#include "GasSource.hpp"
#include "gaden/internal/MathUtils.hpp"

namespace gaden
{
    class BoxSource : public GasSource
    {
    public:
        Vector3 Emit() const override
        {
            Vector3 offset(uniformRandom(-size.x * 0.5, size.x * 0.5),
                           uniformRandom(-size.y * 0.5, size.y * 0.5),
                           uniformRandom(-size.z * 0.5, size.z * 0.5));

            return sourcePosition + offset;
        }

        const char* Type() const override {return "box";} 
        // the sourcePosition is the center of the box
        Vector3 size = {0.5, 0.5, 0.5};
    };
} // namespace gaden