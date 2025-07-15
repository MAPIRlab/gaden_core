#pragma once
#if GPU_ACCELERATION

#define CL_TARGET_OPENCL_VERSION 300
#include "boost/compute.hpp" // IWYU pragma: keep
#include "gaden/Environment.hpp"
#include "gaden/datatypes/Filament.hpp"
#include "gaden/datatypes/SimulationMetadata.hpp"
#include <optional>

namespace compute = boost::compute;

namespace gaden
{

    struct ComputeConcentrationCommand
    {
        Vector3i indices;
        uint32_t filament;
    };

    class GPUAcceleration
    {
    public:
        GPUAcceleration();
        ~GPUAcceleration();
        void Setup(class Environment const& env, SimulationMetadata::Constants const& constants);
        void UpdateConcentrations(std::vector<ComputeConcentrationCommand> const& commandsHost,
                                  std::vector<float>& concentrationsHost,
                                  std::vector<Filament> const& filamentsHost);

    private:
        compute::device gpu = compute::system::default_device();
        compute::context context;
        compute::command_queue queue;

        std::optional<compute::kernel> concentrationsKernel;

        compute::vector<Environment::CellState> envData;
        compute::vector<float> concentrationsGPU;
    };
} // namespace gaden

#else
// empty definitions just so we can have the forward declarations in the RunningSimulation header
namespace gaden
{
    struct ComputeConcentrationCommand
    {
    };

    class GPUAcceleration
    {
    };
} // namespace gaden
#endif