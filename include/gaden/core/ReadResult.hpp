#pragma once
#include "Logging.hpp"

namespace gaden
{
    enum class ReadResult
    {
        OK,
        NO_FILE,
        READING_FAILED
    };

#define GADEN_CHECK_RESULT(expression)                                                               \
    {                                                                                          \
        gaden::ReadResult readResult = expression;                                             \
        if (readResult == gaden::ReadResult::NO_FILE)                                                 \
        {                                                                                      \
            GADEN_ERROR("File could not be found! At '{}':{}", __FILE__, __LINE__);            \
            GADEN_TERMINATE;                                                                   \
        }                                                                                      \
        else if (readResult == gaden::ReadResult::READING_FAILED)                                     \
        {                                                                                      \
            GADEN_ERROR("File could not be parsed correctly! At '{}':{}", __FILE__, __LINE__); \
            GADEN_TERMINATE;                                                                   \
        }                                                                                      \
    }
} // namespace gaden