#pragma once

#include <cassert>

#ifdef VX_DEBUG

#define VX_ASSERT(condition) assert(condition)

#else

#define VX_ASSERT(condition)

#endif