#ifndef DAVID_MACROS_H
#define DAVID_MACROS_H

#ifndef warn_unused
#if defined(__GNUC__) || defined(__clang__)
#define warn_unused __attribute__ ((warn_unused_result))
#elif defined(_MSC_VER)
#define warn_unused _Check_return
#else
#error "No warn unused analogue"
#endif
#endif

#include <assert.h>

#if defined(__GNUC__) || defined(__clang__)
#define force_inline __attribute__((always_inline))
#define never_inline __attribute__((noinline))
#else
#define force_inline
#define never_inline
#endif

#ifndef __cplusplus
#define auto __auto_type
#endif

#ifndef __clang__
#define _Nonnull
#define _Nullable
#define _Null_unspecified
#endif

#define Nonnull(x) x _Nonnull
#define Nullable(x) x _Nullable
#define NullUnspec(x) x _Null_unspecified

#ifdef __GNUC__
#define unreachable() __builtin_unreachable()
#else
#define unreachable() assert(0)
#endif

#define arrlen(arr) (sizeof(arr) / sizeof((arr)[0]))

/*
 * Warning Suppression
 *
 * Pragmas to suppress warnings.
 */
#ifdef __clang__
#define SuppressNullabilityComplete() _Pragma("clang diagnostic ignored \"-Wnullability-completeness\"")
#define SuppressUnusedFunction()  _Pragma("clang diagnostic ignored \"-Wunused-function\"")
#define SuppressCastQual()  _Pragma("clang diagnostic ignored \"-Wcast-qual\"")
#define SuppressDiscardQualifiers() _Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"")
#define SuppressMissingBraces()  _Pragma("clang diagnostic ignored \"-Wmissing-braces\"")
#define SuppressDoublePromotion() _Pragma("clang diagnostic ignored \"-Wdouble-promotion\"")
#define SuppressCastFunction()
#define SuppressCoveredSwitchDefault()  _Pragma("clang diagnostic ignored \"-Wcovered-switch-default\"")
#define PushDiagnostic() _Pragma("clang diagnostic push")
#define PopDiagnostic() _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define SuppressNullabilityComplete()
#define SuppressUnusedFunction()
#define SuppressCastQual() _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#define SuppressDiscardQualifiers() _Pragma("GCC diagnostic ignored \"-Wdiscarded-qualifiers\"")
#define SuppressDoublePromotion() _Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")
#define SuppressMissingBraces()  _Pragma("GCC diagnostic ignored \"-Wmissing-braces\"")
#define SuppressCastFunction() _Pragma("GCC diagnostic ignored \"-Wcast-function-type\"")
#define SuppressCoveredSwitchDefault()
#define PushDiagnostic() _Pragma("GCC diagnostic push")
#define PopDiagnostic() _Pragma("GCC diagnostic pop")
#else
#define SuppressNullabilityComplete()
#define SuppressUnusedFunction()
#define SuppressCastQual()
#define SuppressCastFunction()
#define SuppressMissingBraces()
#define SuppressDiscardQualifiers()
#define SuppressDoublePromotion()
#define SuppressCoveredSwitchDefault()
#define PushDiagnostic()
#define PopDiagnostic()
#endif

#if defined(__GNUC__) || defined(__clang__)
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#endif
