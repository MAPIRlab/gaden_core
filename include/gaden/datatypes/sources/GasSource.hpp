#pragma once
#include "gaden/core/Vectors.hpp"
#include "gaden/datatypes/Filament.hpp"
#include "gaden/datatypes/GasTypes.hpp"

// we need to do a forward declaration because gaden does not export yaml-cpp as a dependency
// (to avoid version conflicts on ros nodes that use the yaml-cpp vendor package)
namespace YAML
{
    class Node;
    class Emitter;
} // namespace YAML

namespace gaden
{
    class GasSource
    {
    public:
        virtual Filament Emit() const = 0;
        virtual const char* Type() const = 0;

        Vector3 sourcePosition;
        GasType gasType = GasType::unknown; // Gas type to simulate

        float initialSigma = 10.0;
        float ppmCenter = 20.0;        //[ppm] Gas concentration at the center of the 3D gaussian (filament)
        float numFilaments_sec = 10.0; // How many filaments to release per second

        static std::shared_ptr<GasSource> ParseYAML(YAML::Node const& node);
        static void WriteYAML(YAML::Emitter& emitter, std::shared_ptr<GasSource> source);
    };
} // namespace gaden