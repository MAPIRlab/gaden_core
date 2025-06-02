#pragma once
#include "gaden/core/Vectors.hpp"
#include "gaden/datatypes/GasTypes.hpp"
#include "gaden/datatypes/LoopConfig.hpp"
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
} // namespace gaden