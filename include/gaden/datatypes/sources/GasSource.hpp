#pragma once
#include "gaden/core/Vectors.hpp"
#include "gaden/datatypes/GasTypes.hpp"
#include "gaden/internal/BufferUtils.hpp"

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
        virtual Vector3 Emit() const = 0;
        virtual const char* Type() const = 0;

        static std::shared_ptr<GasSource> ParseYAML(YAML::Node const& node);
        static void WriteYAML(YAML::Emitter& emitter, std::shared_ptr<GasSource> source);
        static void SerializeBinary(BufferWriter& writer, std::shared_ptr<GasSource> source);
        static void DeserializeBinary(BufferReader& reader, std::shared_ptr<GasSource>& source);

        Vector3 sourcePosition;
        GasType gasType = GasType::unknown; // Gas type to simulate
    };
} // namespace gaden