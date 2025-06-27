#pragma once

#include "GasSource.hpp"

namespace gaden
{
    class PointSource : public GasSource
    {
    public:
        virtual Vector3 Emit() const override
        {
            return sourcePosition;
        }  
        const char* Type() const override {return "point";} 

    };
} // namespace gaden