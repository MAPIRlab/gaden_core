#pragma once
#include <cstddef>

namespace gaden
{
    struct LoopConfig
    {
        bool loop = false;
        size_t from = 0;
        size_t to = 0;
    };
} // namespace gaden