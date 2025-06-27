#pragma once

#include "GasSource.hpp"

namespace gaden
{
    class PointSource : public GasSource
    {
    public:
        virtual Filament Emit() const override
        {
            Filament filament(sourcePosition, initialSigma);
            return filament;
        }  
        const char* Type() const override {return "point";} 

    };
} // namespace gaden