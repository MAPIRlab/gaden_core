#pragma once
#include "gaden/core/Vectors.hpp"
#include "gaden/datatypes/GasTypes.hpp"
#include "gaden/datatypes/LoopConfig.hpp"
#include "gaden/datatypes/Model3D.hpp"
#include <yaml-cpp/yaml.h>

namespace YAML
{
    template <> struct convert<gaden::Vector3>
    {
        static Node encode(const gaden::Vector3& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, gaden::Vector3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3)
            {
                return false;
            }

            rhs.x = (node[0].as<double>());
            rhs.y = (node[1].as<double>());
            rhs.z = (node[2].as<double>());
            return true;
        }
    };

    template <> struct convert<gaden::GasType>
    {
        static Node encode(const gaden::GasType& rhs)
        {
            Node node;
            node.push_back(static_cast<size_t>(rhs));
            return node;
        }

        static bool decode(const Node& node, gaden::GasType& rhs)
        {
            try
            {
                rhs = static_cast<gaden::GasType>(node.as<size_t>());
                return true;
            }
            catch (std::exception const& e)
            {
                GADEN_ERROR("Failed to parse gasType from YAML: '{}'", YAML::Dump(node));
                return false;
            }
        }
    };
} // namespace YAML

namespace gaden
{
    template <typename T>
    inline void FromYAML(YAML::Node const& node, std::string const& key, T& value)
    {
        if (auto param = node[key])
            value = param.as<T>();
    }

    inline LoopConfig ParseLoopYAML(YAML::Node const& node)
    {
        LoopConfig config;
        FromYAML<bool>(node, "loop", config.loop);
        FromYAML<size_t>(node, "from", config.from);
        FromYAML<size_t>(node, "to", config.to);
        return config;
    }

    inline void EncodeModelsList(YAML::Emitter& emitter, const std::vector<gaden::Model3D>& models)
    {
        emitter << YAML::BeginSeq;
        gaden::Color color;

        // make sure the file has the right format for hand editing, rather than the empty list symbol
        if (models.size() == 0)
            emitter << "";

        for (size_t i = 0; i < models.size(); i++)
        {
            gaden::Model3D const& model = models[i];
            try
            {
                if (model.color != color)
                {
                    emitter << fmt::format("!color [{:.2f}, {:.2f}, {:.2f}]", model.color.r, model.color.g, model.color.b);
                    color = model.color;
                }
                emitter << std::filesystem::canonical(model.path).c_str();
            }
            catch (std::exception const& e)
            {
                GADEN_WARN("Ignoring model '{}'. Reason: {}", model.path, e.what());
            }
        }
        emitter << YAML::EndSeq;
    }

    static bool DecodeModelsList(const YAML::Node& node, std::vector<gaden::Model3D>& models)
    {
        try
        {
            gaden::Color color;
            for (size_t i = 0; i < node.size(); i++)
            {
                std::string line = node[i].as<std::string>();
                if (line.starts_with('!'))
                    color = Color::Parse(line);
                else
                    models.push_back({.path = line, .color = color});
            }
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Failed to parse models list");
            return false;
        }
        return true;
    }
} // namespace gaden