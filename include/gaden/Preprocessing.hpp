#pragma once

#include "gaden/Environment.hpp"
#include "gaden/EnvironmentConfigMetadata.hpp"
#include "gaden/internal/Triangle.hpp"
#include "gaden/internal/WindSequence.hpp"

namespace gaden
{
    class Preprocessing
    {
    public:
        static Environment ParseSTLModels(const std::vector<std::filesystem::path>& mainModels,
                                          const std::vector<std::filesystem::path>& outletModels,
                                          float cellSize,
                                          Vector3 emptyPoint);

        static WindSequence ParseOpenFoamVectorCloud(const std::vector<std::filesystem::path>& files,
                                                     const Environment& env,
                                                     LoopConfig loopConfig);

        static std::optional<EnvironmentConfiguration> Preprocess(EnvironmentConfigMetadata const& metadata);

    private:
        struct BoundingBox
        {
            Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
            Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

            void Grow(const Vector3& point);
            void Grow(const BoundingBox& other);
        };

        static Preprocessing::BoundingBox findDimensions(const std::vector<Triangle>& triangles);
        static void occupy(std::vector<Triangle>& triangles, Environment& env, Environment::CellState value_to_write);
        static void fill(Environment& environment, Vector3 empty_point);
    };
} // namespace gaden