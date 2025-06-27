#pragma once

#include "gaden/core/Vectors.hpp"
#include "gaden/datatypes/GasTypes.hpp"
#include "gaden/datatypes/sources/GasSource.hpp"

namespace gaden
{
    struct SimulationMetadata
    {
        std::shared_ptr<GasSource> source;

        // constants used to calculate the concentration of gas from the filament positions
        // they are derived from the temperature, pressure, and initial gas concentration of the filaments
        struct Constants
        {
            float totalMolesInFilament;
            float numMolesAllGasesIncm3;
        } constants;
    };
} // namespace gaden