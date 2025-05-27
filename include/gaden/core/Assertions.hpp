#pragma once
#include "Logging.hpp"

#define GADEN_TERMINATE raise(SIGTRAP)

// verify is always available, including on release
#define GADEN_VERIFY(cnd, msg)                                                                                                                    \
    if (!(cnd))                                                                                                                                   \
    {                                                                                                                                             \
        GADEN_ERROR("{0}:     At {1}",                                                                                                            \
                    fmt::format(fmt::bg(fmt::terminal_color::red) | fmt::fg(fmt::terminal_color::white) | fmt::emphasis::bold, "ERROR: {}", msg), \
                    fmt::format(fmt::emphasis::bold, "{0}:{1}", __FILE__, __LINE__));                                                             \
        GADEN_TERMINATE;                                                                                                                          \
    }

// asserts are only active on debug builds
#if GADEN_DEBUG
#define GADEN_ASSERT(cnd, msg) GADEN_VERIFY(cnd, msg)
#else
#define GADEN_ASSERT(cnd, msg)
#endif
