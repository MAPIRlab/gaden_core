#pragma once

#include "gaden/Environment.hpp"
#include "gaden/internal/WindSequence.hpp"
#include "gaden/internal/Triangle.hpp"

namespace gaden
{
    class Preprocessing
    {
    public:
        static Environment ParseSTLModels(const std::vector<std::filesystem::path>& mainModels,
                                          const std::vector<std::filesystem::path>& outletModels);

        static WindSequence ParseOpenFoamVectorCloud(const std::vector<std::filesystem::path>& files);

    private:
        struct BoundingBox
        {
            Vector3 min, max;
            void Grow(const Vector3& point);
            void Grow(const BoundingBox& other);
        };

        void occupy(std::vector<Triangle>& triangles, Environment& env, Environment::CellState value_to_write);

        static BoundingBox findDimensions(const std::filesystem::path& model);
    };
} // namespace gaden