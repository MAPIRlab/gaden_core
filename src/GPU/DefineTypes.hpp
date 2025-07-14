#pragma once 

#include "gaden/Environment.hpp"
#include "gaden/datatypes/Filament.hpp"
#include "gaden/datatypes/SimulationMetadata.hpp"
#include <boost/compute/types/struct.hpp>
#include <boost/compute/utility/source.hpp>
#include "gaden/internal/GPUAcceleration.hpp"

// in OpenCL, float3 is actually float4 in disguise, which means sending a Vector3 as a float3 messes everything up
// sadly, that means that we cannot do arithmetic on a vector3 directly inside the gpu
// however, we can extract a proper float3 by using vload3(0, &Vector3.x)
BOOST_COMPUTE_ADAPT_STRUCT(gaden::Vector3, Vector3, (x,y,z));
BOOST_COMPUTE_ADAPT_STRUCT(gaden::Vector3i, Vector3i, (x,y,z));

BOOST_COMPUTE_ADAPT_STRUCT(gaden::Environment::Description, EnvironmentDescription, (dimensions, minCoord, maxCoord, cellSize));
BOOST_COMPUTE_ADAPT_STRUCT(gaden::Filament, Filament, (position, sigma));
BOOST_COMPUTE_ADAPT_STRUCT(gaden::SimulationMetadata::Constants, Constants, (totalMolesInFilament, numMolesAllGasesIncm3));
BOOST_COMPUTE_ADAPT_STRUCT(gaden::ComputeConcentrationCommand, ComputeConcentrationCommand, (indices, filament));

inline const char* VectorConversion = BOOST_COMPUTE_STRINGIZE_SOURCE(
    float3 readf3(Vector3 v) {return vload3(0, &v.x);}
    void writef3(float3 f, Vector3* v){vstore3(f, 0, &v->x);}

    int3 readi3(Vector3i v) {return vload3(0, &v.x);}
    void writei3(int3 i, Vector3i* v){vstore3(i, 0, &v->x);}
);

