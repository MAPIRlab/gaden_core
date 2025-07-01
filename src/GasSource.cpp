#include "YAML_Conversions.hpp"
#include "gaden/datatypes/sources/BoxSource.hpp"
#include "gaden/datatypes/sources/CylinderSource.hpp"
#include "gaden/datatypes/sources/LineSource.hpp"
#include "gaden/datatypes/sources/PointSource.hpp"
#include "gaden/datatypes/sources/SphereSource.hpp"
#include "gaden/internal/Pointers.hpp"
#include "yaml-cpp/emitter.h"

namespace gaden
{
    std::shared_ptr<GasSource> GasSource::ParseYAML(YAML::Node const& node)
    {
        try
        {
            std::string sourceType = "";
            FromYAML(node, "sourceType", sourceType);

            std::shared_ptr<GasSource> source;
            if (sourceType == "point")
            {
                source = std::make_shared<PointSource>();
            }
            else if (sourceType == "box")
            {
                source = std::make_shared<BoxSource>();
                As<BoxSource>(source)->size = node["size"].as<Vector3>();
            }
            else if (sourceType == "line")
            {
                source = std::make_shared<LineSource>();
                As<LineSource>(source)->lineEnd = node["lineEnd"].as<Vector3>();
            }
            else if (sourceType == "sphere")
            {
                source = std::make_shared<SphereSource>();
                As<SphereSource>(source)->radius = node["radius"].as<float>();
            }
            else if (sourceType == "cylinder")
            {
                source = std::make_shared<CylinderSource>();
                As<CylinderSource>(source)->radius = node["radius"].as<float>();
                As<CylinderSource>(source)->height = node["height"].as<float>();
            }
            else
            {
                GADEN_WARN("Invalid source type '{}'. Creating point source as a default.", sourceType);
                source = std::make_shared<PointSource>();
            }

            // common
            source->sourcePosition = node["position"].as<Vector3>();
            source->gasType = node["gasType"].as<GasType>();

            return source;
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Exception while parsing source description: '{}'", e.what());
            return std::make_shared<PointSource>();
        }
    }

    void GasSource::WriteYAML(YAML::Emitter& emitter, std::shared_ptr<GasSource> source)
    {
        emitter << YAML::BeginMap;

        if (Is<PointSource>(source))
        {
            emitter << YAML::Key << "sourceType" << YAML::Value << "point";
        }
        else if (Is<BoxSource>(source))
        {
            emitter << YAML::Key << "sourceType" << YAML::Value << "box";
            emitter << YAML::Key << "size" << YAML::Value << As<BoxSource>(source)->size;
        }
        else if (Is<LineSource>(source))
        {
            emitter << YAML::Key << "sourceType" << YAML::Value << "line";
            emitter << YAML::Key << "lineEnd" << YAML::Value << As<LineSource>(source)->lineEnd;
        }
        else if (Is<SphereSource>(source))
        {
            emitter << YAML::Key << "sourceType" << YAML::Value << "sphere";
            emitter << YAML::Key << "radius" << YAML::Value << As<SphereSource>(source)->radius;
        }
        else if (Is<CylinderSource>(source))
        {
            emitter << YAML::Key << "sourceType" << YAML::Value << "cylinder";
            emitter << YAML::Key << "radius" << YAML::Value << As<CylinderSource>(source)->radius;
            emitter << YAML::Key << "height" << YAML::Value << As<CylinderSource>(source)->height;
        }
        else
        {
            GADEN_ERROR("Serialization for this source type is not implemented!");
        }

        emitter << YAML::Key << "position" << YAML::Value << YAML::Flow << source->sourcePosition;
        emitter << YAML::Key << "gasType" << YAML::Value << source->gasType;

        emitter << YAML::EndMap;
    }

    // we cant do simple serialization with memcpy because of the dynamic types
    // we *could*, in theory memcpy from the first field of the object (having already cast to the concrete type) to avoid the vpointer
    // but that layout is technically a compiler-dependent thing, so... we'll just serialize each field independently

    void GasSource::SerializeBinary(BufferWriter& writer, std::shared_ptr<GasSource> source)
    {
        std::string sourceType = source->Type();
        writer.Write(&sourceType);

        if (sourceType == "point")
        {
        }
        else if (sourceType == "box")
        {
            writer.Write(&As<BoxSource>(source)->size);
        }
        else if (sourceType == "line")
        {
            writer.Write(&As<LineSource>(source)->lineEnd);
        }
        else if (sourceType == "sphere")
        {
            writer.Write(&As<SphereSource>(source)->radius);
        }
        else if (sourceType == "cylinder")
        {
            writer.Write(&As<CylinderSource>(source)->radius);
            writer.Write(&As<CylinderSource>(source)->height);
        }
        else
        {
            GADEN_ERROR("Invalid source type '{}'. Cannot serialize it", sourceType);
            GADEN_TERMINATE;
        }

        writer.Write(&source->sourcePosition);
        writer.Write(&source->gasType);
    }

    void GasSource::DeserializeBinary(BufferReader& reader, std::shared_ptr<GasSource>& source)
    {
        std::string sourceType;
        reader.Read(&sourceType);

        if (sourceType == "point")
        {
            if (!Is<PointSource>(source))
                source = std::make_shared<PointSource>();
        }
        else if (sourceType == "box")
        {
            if (!Is<BoxSource>(source))
                source = std::make_shared<BoxSource>();
            reader.Read(&As<BoxSource>(source)->size);
        }
        else if (sourceType == "line")
        {
            if (!Is<LineSource>(source))
                source = std::make_shared<LineSource>();
            reader.Read(&As<LineSource>(source)->lineEnd);
        }
        else if (sourceType == "sphere")
        {
            if (!Is<SphereSource>(source))
                source = std::make_shared<SphereSource>();
            reader.Read(&As<SphereSource>(source)->radius);
        }
        else if (sourceType == "cylinder")
        {
            if (!Is<CylinderSource>(source))
                source = std::make_shared<CylinderSource>();
            reader.Read(&As<CylinderSource>(source)->radius);
            reader.Read(&As<CylinderSource>(source)->height);
        }
        else
        {
            GADEN_ERROR("Invalid source type '{}'. Failed deserialization", sourceType);
            GADEN_TERMINATE;
        }

        reader.Read(&source->sourcePosition);
        reader.Read(&source->gasType);
    }
} // namespace gaden