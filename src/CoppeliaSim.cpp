#include "gaden/integration/CoppeliaSim.hpp"
#include "gaden/core/Logging.hpp"

#if COPPELIA_REMOTE_API
#define SIM_REMOTEAPICLIENT_OBJECTS
#include <RemoteAPIClient.h>
#endif

namespace gaden
{
    void GenerateCoppeliaScene(std::vector<Model3D> const& models, std::filesystem::path const& outputPath)
    {
#if COPPELIA_REMOTE_API
        RemoteAPIClient client;
        RemoteAPIObject::sim sim = client.getObject().sim();
        sim.stopSimulation();
        while (sim.getSimulationState() != sim.simulation_stopped)
            ;

        for (Model3D const& model : models)
        {
            int result = -1;
            while (result == -1)
                result = client.getObject().sim().importShape(0, model, 0, 0.0001f, 1);
        }

        client.getObject().sim().saveScene(outputPath);
#else
        GADEN_ERROR("Gaden was not compiled with coppelia support. Have a look at the CMakeLists.txt");
#endif
    }
} // namespace gaden
