#pragma once

#include "gaden/core/Vectors.hpp"
#include "gaden/datatypes/GasTypes.hpp"

namespace gaden
{
    struct SimulationMetadata
    {
        Vector3 sourcePosition;
        GasType gasType = GasType::unknown; // Gas type to simulate
        
        // constants used to calculate the concentration of gas from the filament positions
        // they are derived from the temperature, pressure, and initial gas concentration of the filaments
        float totalMolesInFilament;
        float numMolesAllGasesIncm3;
    };
} // namespace gaden