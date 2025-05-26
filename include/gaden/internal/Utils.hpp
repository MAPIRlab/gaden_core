#pragma once
#include "gaden/core/Vectors.hpp"
#include "gaden/core/Logging.hpp"
#include <filesystem>
#include <random>

namespace gaden
{
    inline size_t indexFrom3D(const Vector3i& index, const Vector3i& numCellsEnv)
    {
        return index.x + index.y * numCellsEnv.x + index.z * numCellsEnv.x * numCellsEnv.y;
    }

    inline bool InRange(int val, int min, int max)
    {
        return val >= min && val < max;
    }

    // thread-safe
    inline float GaussianRandom(float mean, float stdDev)
    {
        static thread_local std::mt19937 engine;
        static thread_local std::normal_distribution<> dist{0, 1};
        return mean + dist(engine) * stdDev;
    }

    inline float uniformRandom(float min, float max)
    {
        static thread_local std::mt19937 engine;
        static thread_local std::uniform_real_distribution<float> distribution{0.0, 1.0};
        return min + distribution(engine) * (max - min);
    }

    inline bool Approx(float x, float y, float epsilon=1e-5)
    {
        return std::abs(x - y) < epsilon;
    }

    inline void TryCreateDirectory(std::filesystem::path const& path)
    {
        if (!std::filesystem::exists(path))
        {
            if (!std::filesystem::create_directory(path))
            {
                GADEN_WARN("Cannot create directory '{}'", path);
                GADEN_TERMINATE;
            }
        }
    }
} // namespace gaden