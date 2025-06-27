#include "YAML_Conversions.hpp"
#include "gaden/datatypes/sources/BoxSource.hpp"
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
                As<SphereSource>(source)->SetRadius(node["radius"].as<float>());
            }
            else
            {
                GADEN_WARN("Invalid source type '{}'. Creating point source as a default.", sourceType);
                source = std::make_shared<PointSource>();
            }

            // common
            source->sourcePosition = node["position"].as<Vector3>();
            source->gasType = node["gasType"].as<GasType>();
            source->initialSigma = node["initialSigma"].as<float>();
            source->ppmCenter = node["ppmCenter"].as<float>();
            source->numFilaments_sec = node["numFilaments_sec"].as<float>();

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
            emitter << YAML::Key << "radius" << YAML::Value << As<SphereSource>(source)->GetRadius();
        }
        else
        {
            GADEN_ERROR("Serialization for this source type is not implemented!");
        }

        emitter << YAML::Comment("common");
        emitter << YAML::Key << "position" << YAML::Value << source->sourcePosition;
        emitter << YAML::Key << "gasType" << YAML::Value << source->gasType;
        emitter << YAML::Key << "initialSigma" << YAML::Value << source->initialSigma;
        emitter << YAML::Key << "ppmCenter" << YAML::Value << source->ppmCenter;
        emitter << YAML::Key << "numFilaments_sec" << YAML::Value << source->numFilaments_sec;

        emitter << YAML::EndMap;
    }

} // namespace gaden