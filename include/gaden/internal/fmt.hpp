#pragma once

// some versions of GCC produce a long warning that is known to be a false positive inside the fmt headers
// so, check if we are using GCC, and if so temporarily suppress that warning type
#if (defined(__GNUC__) && !defined(__clang__))
#define GCC_COMPILER 1
#else
#define GCC_COMPILER 0
#endif

#if GCC_COMPILER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#include <fmt/color.h>
#include <fmt/format.h>

#if GCC_COMPILER
#pragma GCC diagnostic pop
#endif