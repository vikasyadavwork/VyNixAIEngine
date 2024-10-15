#pragma once

// ------------------------------------------------------------
// Compiler
// ------------------------------------------------------------

#if defined(_MSC_VER)
#define VX_COMPILER_MSVC
#elif defined(__clang__)
#define VX_COMPILER_CLANG
#elif defined(__GNUC__)
#define VX_COMPILER_GCC
#else
#error Unsupported compiler
#endif

// ------------------------------------------------------------
// Build Configuration
// ------------------------------------------------------------

#ifdef NDEBUG
#define VX_RELEASE
#else
#define VX_DEBUG
#endif

// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------

#define VX_BIT(x) (1 << (x))
#define VX_UNUSED(x) (void)(x)

#if defined(_MSC_VER)
#define VX_FORCE_INLINE __forceinline
#else
#define VX_FORCE_INLINE inline __attribute__((always_inline))
#endif
