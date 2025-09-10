#include "gaden/integration/Gazebo.hpp"
#include <fstream>

namespace gaden
{
    void GenerateGazeboScene(Environment const& env, std::vector<Model3D> const& models, std::filesystem::path const& outputFolder)
    {
        std::filesystem::path filepath = outputFolder / "gaden.sdf";
        std::ofstream file(filepath);
        if (!file.is_open())
        {
            GADEN_ERROR("Could not create file '{}'", filepath);
            return;
        }

        file << R"(
<?xml version="1.0" ?>
<sdf version="1.8">
    <world name="world_demo">
)";

        for (auto const& model : models)
        {
            file << fmt::format(
                        "       <model name=\"{}\">", model.path.stem())
                 <<
                R"(
        <static>true</static>
        <link name="link_0">
        <visual name="visual">
            <geometry>
            <mesh>
                <uri>)";
            file << model.path.c_str();
            file << R"(</uri>
            </mesh>
            </geometry>
        </visual>
        <collision name="collision_0">
            <geometry>
            <mesh>
                <uri>)";
            file << model.path.c_str();
            file << R"(</uri>
            </mesh>
            </geometry>
        </collision>
        </link>
    </model>
)";
        }

        file << R"(
    </world>
</sdf>
)";
    }
} // namespace gaden