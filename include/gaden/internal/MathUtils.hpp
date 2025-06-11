#pragma once
#include "gaden/core/Logging.hpp"
#include "gaden/core/Vectors.hpp"
#include <random>

namespace gaden
{
    constexpr float Deg2Rad = M_PI / 180.f;
    constexpr float Rad2Deg = 180.f / M_PI;

    inline size_t indexFrom3D(const Vector3i& index, const Vector3i& numCellsEnv)
    {
        return index.x + index.y * numCellsEnv.x + index.z * numCellsEnv.x * numCellsEnv.y;
    }

    inline Vector3i indicesFrom1D(size_t index, const Vector3i& numCellsEnv)
    {
        size_t z = index / (numCellsEnv.x * numCellsEnv.y);
        size_t remainder = index % (numCellsEnv.x * numCellsEnv.y);
        return Vector3i(remainder % numCellsEnv.x, remainder / numCellsEnv.x, z);
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

    inline bool Approx(float x, float y, float epsilon = 1e-5)
    {
        return std::abs(x - y) < epsilon;
    }

    // holds a long list of N(0,1) values, and returns them one at a time, scaled as requested.
    // obviously not as good as generating them on the fly, but it's not like we are doing cryptography here
    template <int Size>
    class PrecalculatedGaussian
    {
    public:
        PrecalculatedGaussian()
        {
            m_index = uniformRandom(0, Size);
            for (size_t i = 0; i < Size; i++)
                m_precalculatedTable[i] = GaussianRandom(0, 1);
        }

        float nextValue(float mean, float stdev)
        {
            m_index = (m_index + 1) % Size;
            return mean + stdev * m_precalculatedTable[m_index];
        }

    private:
        uint16_t m_index;
        std::array<float, Size> m_precalculatedTable;
    };

} // namespace gaden