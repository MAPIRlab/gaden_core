#include <fstream>
#include <gaden/integration/MVSim.hpp>

void gaden::GenerateMVSimScene(Environment const& env, std::vector<Model3D> const& models, std::filesystem::path const& outputFolder)
{
    // adapted from https://github.com/MRPT/mvsim/tree/master/mvsim_tutorial
    // we write XML directly, to avoid adding dependencies to gaden

    std::filesystem::path filepath = outputFolder / "gaden.world.xml";
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        GADEN_ERROR("Could not create file '{}'", filepath);
        return;
    }

    // create the occupancy grid in the same folder. The name of this file is referenced in the world .xml
    env.Write2DSlicePGM(outputFolder / "occupancy.pgm", 0.0, false);
    env.WriteROSOccupancyYAML(outputFolder / "occupancy.yaml", 0.0);

    // emit the xml for the basic configuration
    file << R"(
<mvsim_world version="1.0">
<!-- General simulation options -->
<simul_timestep>0</simul_timestep> <!-- Simulation fixed-time interval for numerical integration [seconds] or 0 to autodetermine -->

<!-- GUI options -->
<gui>
    <ortho>false</ortho>
    <show_forces>true</show_forces>  <force_scale>0.01</force_scale>
    <cam_distance>35</cam_distance>
    <fov_deg>60</fov_deg>
    <refresh_fps>20</refresh_fps>
    <!-- <follow_vehicle>r1</follow_vehicle> -->
</gui>

<!-- Light parameters -->
<lights>
</lights>

)";

    // occupancy grid referenced here
    file << R"(
<!--========================
        Scenario definition
    ======================== -->
<element class="occupancy_grid">
    <!-- File can be an image or an MRPT .gridmap file -->
    <file>occupancy.yaml</file>

    <!--<show_collisions>true</show_collisions>-->
</element>

<!-- ground grid (for visual reference) -->
<element class="ground_grid">
    <floating>true</floating>
</element>

)";

    // create the mesh objects
    file << R"(
<!-- ======================================
     Object types
     ====================================== -->
)";

    // for each model, define an object class and instantiate it
    for (const auto& model : models)
    {
        // clang-format off

        //class definition
        std::string name = model.path.stem();
        file << R"(
<block:class name=")" << name << R"(">
    <static>true</static> <!-- Does not move -->
    <shape_from_visual/> <!-- automatic shape,zmin,zmax from 3D mesh-->
    <!--  Custom visualization model. 3D model filename to load (local or remote http://uri ) -->
    <visual>
)"
"        <model_uri>" << std::filesystem::canonical(model.path).c_str() << "</model_uri>" << R"(
    </visual>
</block:class>

)";

        //instantiation
        file << "<block class=\""<< name <<"\"> <init_pose>0 0 0</init_pose> </block>\n\n\n";
        // clang-format on
    }

    file << R"(
<!-- ======================================
     Define your vehicles here
     ====================================== -->


)";

    file << "</mvsim_world>";
    file.close();
    GADEN_INFO("Created MVSim file at '{}'", filepath);
}
