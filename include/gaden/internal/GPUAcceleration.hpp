#pragma once
#define CL_TARGET_OPENCL_VERSION 300
#include "boost/compute.hpp"
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