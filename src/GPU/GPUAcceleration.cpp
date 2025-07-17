#if GPU_ACCELERATION

#include "gaden/internal/GPUAcceleration.hpp"
#include "DefineTypes.hpp"
#include "gaden/core/Assertions.hpp"
#include "gaden/core/Logging.hpp"
#include <boost/compute/container/vector.hpp>
#include <boost/compute/types/struct.hpp>
#include <gaden/internal/Profiling.hpp>

namespace gaden
{
    GPUAcceleration::GPUAcceleration() : context(gpu),
                                         queue(context, gpu),
                                         envData(context),
                                         concentrationsGPU(context)
    {}

    GPUAcceleration::~GPUAcceleration()
    {
        GADEN_INFO("Releasing GPU resources");
    }

    void GPUAcceleration::Setup(Environment const& env, SimulationMetadata::Constants const& constants)
    {
        // Source code
        //-----------------------------
        std::string source = BOOST_COMPUTE_STRINGIZE_SOURCE(

            // OpenCL float atomicadd hack:
            // http://suhorukov.blogspot.co.uk/2011/12/opencl-11-atomic-operations-on-floating.html
            // https://devtalk.nvidia.com/default/topic/458062/atomicadd-float-float-atomicmul-float-float-/
            inline float atomicadd(volatile __global float* address, const float value) {
                float old = value, orig;
                while ((old = atomic_xchg(address, (orig = atomic_xchg(address, 0.0f)) + old)) != 0.0f)
                    ;
                return orig;
            }

            typedef enum {
                Free = 0,        // cell is empty, gas can be here
                Obstacle = 1,    // cell is occupied by an obstacle, no filaments can go through it
                Outlet = 2,      // if a filament enters this cell it is removed from the simulation
                OutOfBounds = 3, // invalid cell, position is out of the map bounds
                Uninitialized = 255} CellState;

            uint indexFrom3D(Vector3i index, Vector3i numCellsEnv) {
                return index.x + index.y * numCellsEnv.x + index.z * numCellsEnv.x * numCellsEnv.y;
            }

            float3 coordsOfCellCenter(Vector3i indices, EnvironmentDescription description) {
                return readf3(description.minCoord) + (convert_float3(readi3(indices)) + 0.5f) * description.cellSize;
            }

            Vector3i coordsToIndices(float3 coords, EnvironmentDescription description) {
                float3 ind = (coords - readf3(description.minCoord)) / description.cellSize;
                return (Vector3i){ind.x, ind.y, ind.z};
            }

            bool CheckLineOfSight(float3 start, float3 end, EnvironmentDescription description, uchar * envData) {
                // Check whether one of the points is outside the valid environment or is not free
                Vector3i startInd = coordsToIndices(start, description);
                Vector3i endInd = coordsToIndices(end, description);
                if (envData[indexFrom3D(startInd, description.dimensions)] != Free || envData[indexFrom3D(endInd, description.dimensions)] != Free)
                    return false;

                // Calculate displacement vector
                float3 vector = end - start;
                float distance = length(vector);
                vector = vector / distance;

                // Traverse path
                int steps = distance / description.cellSize; // Make sure no two iteration steps are separated more than 1 cell
                float increment = distance / steps;

                for (int i = 1; i < steps; i++)
                {
                    // Determine point in space to evaluate
                    float3 position = start + vector * (increment * i);

                    // Check if the cell is occupied
                    Vector3i indices = coordsToIndices(position, description);
                    if (envData[indexFrom3D(indices, description.dimensions)] != Free)
                        return false;
                }

                // Direct line of sight confirmed!
                return true;
            }

            constant float pi_cubed = M_PI * M_PI * M_PI;
            float ConcentrationAtCenter(Filament filament, Constants constants) {
                float numMolesTarget_cm3 = constants.totalMolesInFilament //
                                           / (sqrt(8 * pi_cubed) * filament.sigma * filament.sigma * filament.sigma);

                return 1e6 * numMolesTarget_cm3 / constants.numMolesAllGasesIncm3; // express in ppm
            }

            float CalculateConcentrationSingleFilament(Filament filament, float3 samplePoint, Constants constants) {
                // calculate how much gas concentration does one filament contribute to the queried location
                float sigma = filament.sigma;
                float distance_cm = 100 * length(readf3(filament.position) - samplePoint);

                float ppm = ConcentrationAtCenter(filament, constants) *
                            exp(-(distance_cm * distance_cm) / (2 * sigma * sigma));
                return ppm;
            }

            // KERNEL
            __kernel void test(global ComputeConcentrationCommand * commands,
                               global Filament * filaments,
                               global atomic_float * concentrations,
                               EnvironmentDescription envDesc,
                               global uchar * envData,
                               Constants constants) {
                int i = get_global_id(0);

                ComputeConcentrationCommand command = commands[i];
                Filament filament = filaments[command.filament];

                uint cellIndex = indexFrom3D(command.indices, envDesc.dimensions);
                float3 samplePoint = coordsOfCellCenter(command.indices, envDesc);

                if (CheckLineOfSight(readf3(filament.position), samplePoint, envDesc, envData))
                {
                    float c = CalculateConcentrationSingleFilament(filament, samplePoint, constants);
                    atomicadd(&concentrations[cellIndex], c);
                }
            });
        //-----------------------------

        source = compute::type_definition<Vector3>() + "\n"                         //
                 + compute::type_definition<Vector3i>() + "\n"                      //
                 + compute::type_definition<Environment::Description>() + "\n"      //
                 + compute::type_definition<Filament>() + "\n"                      //
                 + compute::type_definition<ComputeConcentrationCommand>() + "\n"   //
                 + compute::type_definition<SimulationMetadata::Constants>() + "\n" //
                 + VectorConversion                                                 //
                 + source;

        // GADEN_INFO("SOURCE:\n{}", source);

        compute::program program = compute::program::create_with_source(source, context);
        try
        {
            program.build();
        }
        catch (std::exception const& e)
        {
            GADEN_ERROR("Failed to build OpenCL kernel:\n{}", program.build_log());
            GADEN_TERMINATE;
        }
        concentrationsKernel = program.create_kernel("test");

        ///////////////////////////
        // Set the constant parameters
        ///////////////////////////
        concentrationsGPU.resize(env.cells.size());
        envData.resize(env.cells.size(), queue);
        compute::copy(env.cells.begin(), env.cells.end(), envData.begin(), queue);

        concentrationsKernel->set_arg(2, concentrationsGPU);
        concentrationsKernel->set_arg(3, sizeof(Environment::Description), &env.description);
        concentrationsKernel->set_arg(4, envData.get_buffer());
        concentrationsKernel->set_arg(5, sizeof(SimulationMetadata::Constants), &constants);
        GADEN_INFO("Kernel created");
    }

    void GPUAcceleration::UpdateConcentrations(std::vector<ComputeConcentrationCommand> const& commandsHost,
                                               std::vector<float>& concentrationsHost,
                                               std::vector<Filament> const& filamentsHost)
    {
        ZoneScopedN("ConcentrationsGPU");
        if (commandsHost.empty())
            return;

        compute::fill(concentrationsGPU.begin(), concentrationsGPU.end(), 0, queue);
        compute::vector<ComputeConcentrationCommand> commandsGPU(commandsHost, queue);
        compute::vector<Filament> filamentsGPU(filamentsHost, queue);

        concentrationsKernel->set_arg(0, commandsGPU);
        concentrationsKernel->set_arg(1, filamentsGPU);
        queue.enqueue_1d_range_kernel(*concentrationsKernel, 0, commandsHost.size(), 0);

        compute::copy(concentrationsGPU.begin(), concentrationsGPU.end(), concentrationsHost.begin(), queue);

        queue.finish();
    }
} // namespace gaden

#endif