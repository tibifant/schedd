#pragma once

#define _USE_MATH_DEFINES 1

#include <stdint.h>
#include <climits>
#include <math.h>
#include <functional>
#include <type_traits>
#include <inttypes.h>
#include <float.h>
#include <memory.h>
#include <malloc.h>

#ifndef BIT_FALLBACK
#include <bit>
#endif

#include <chrono>
#include <algorithm>
#include <utility>
#include <concepts>

#ifndef LS_PLATFORM_WINDOWS
#if defined(_WIN64) || defined(_WIN32)
#define LS_PLATFORM_WINDOWS 1
#endif
#endif

#ifndef LS_PLATFORM_UNIX
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#define LS_PLATFORM_UNIX 1
#endif
#endif

#ifndef LS_PLATFORM_APPLE
#if defined(__APPLE__) && defined(__MACH__)
#define LS_PLATFORM_APPLE 1
#endif
#endif

#ifndef LS_PLATFORM_LINUX
#if defined(__linux__)
#define LS_PLATFORM_LINUX 1
#endif
#endif

#ifndef LS_PLATFORM_BSD
#if defined(BSD)
#define LS_PLATFORM_BSD 1
#endif
#endif

#ifndef LS_ARCH_X64
#ifdef LS_PLATFOWM_WINDOWS
#define LS_ARCH_X64 1
#else
#ifdef __x86_64__
#define LS_ARCH_X64 1
#endif
#endif
#endif

#ifndef LS_ARCH_ARM64
#ifdef __ARM_ARCH
#define LS_ARCH_ARM64 1
#endif
#endif

#ifdef LS_PLATFORM_WINDOWS
#include <intrin.h>
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#undef near
#undef far
#undef NEAR
#undef FAR
#else
#ifdef LS_ARCH_X64
#include <x86intrin.h>
#endif
#include <unistd.h>

#define __debugbreak() __builtin_trap()
#endif

#ifndef _In_
#define _In_
#endif

#ifndef _Out_
#define _Out_
#endif

#ifndef _In_Out_
#define _In_Out_
#endif

#ifndef _Out_opt_
#define _Out_opt_
#endif

#ifdef _MSC_VER
#define LS_ALIGN(bytes) __declspec(align(bytes))
#else
#define LS_ALIGN(bytes) __attribute__((aligned(bytes)))
#endif

#if defined(_MSC_VER) || defined(__clang__)
#define LS_VECTORCALL __vectorcall
#else
#define LS_VECTORCALL
#endif

enum lsResult
{
  lsR_Success,

  lsR_Failure,
  lsR_InvalidParameter,
  lsR_ArgumentNull,
  lsR_IOFailure,
  lsR_ArgumentOutOfBounds,
  lsR_ResourceInvalid,
  lsR_ResourceIncompatible,
  lsR_ResourceNotFound,
  lsR_ResourceStateInvalid,
  lsR_ResourceAlreadyExists,
  lsR_ResourceBusy,
  lsR_MemoryAllocationFailure,
  lsR_OperationNotSupported,
  lsR_NotSupported,
  lsR_EndOfStream,
  lsR_InternalError,
  lsR_ResourceFull,
  lsR_ResourceInsufficient,

  _lsResult_Count,
};

#define LS_SUCCESS(errorCode) (errorCode == lsR_Success)
#define LS_FAILED(errorCode) (!(LS_SUCCESS(errorCode)))

#define LS_STRINGIFY(x) #x
#define LS_STRINGIFY_VALUE(x) LS_STRINGIFY(x)

extern thread_local bool _ls_error_silent;
#ifndef RELEASE_BUILD
extern thread_local bool _ls_error_break;
#endif

#ifdef _DEBUG
#define LS_DEBUG_ONLY_BREAK() __debugbreak()
#else
#define LS_DEBUG_ONLY_BREAK() do { } while (0)
#endif

const char *lsResult_to_string(const lsResult result);
bool _ls_error_handle_direct(const lsResult result, const char *expression, const char *file, const uint32_t line);
bool _ls_error_handle_indirect(const lsResult result, const char *expression, const char *file, const uint32_t line);

#define LS_ERROR_SET_INTERNAL(errorCode, text, direct) \
  do \
  { result = errorCode; \
    if constexpr (direct) \
    { if (_ls_error_handle_direct(errorCode, text, __FILE__, __LINE__)) { LS_DEBUG_ONLY_BREAK(); } } \
    else \
    { if (_ls_error_handle_indirect(errorCode, text, __FILE__, __LINE__)) { LS_DEBUG_ONLY_BREAK(); } } \
    goto epilogue; \
  } while (0) 

#define LS_ERROR_SET(errorCode) LS_ERROR_SET_INTERNAL(errorCode, #errorCode, true)

#define LS_ERROR_IF(booleanExpression, errorCode) \
  do \
  { if (booleanExpression) [[unlikely]] \
      LS_ERROR_SET_INTERNAL(errorCode, #booleanExpression, true); \
  } while (0)

#define LS_ERROR_CHECK(functionCall) \
  do \
  { result = (functionCall); \
    if (LS_FAILED(result)) [[unlikely]] \
      LS_ERROR_SET_INTERNAL(result, #functionCall, false); \
  } while (0)

class lsErrorPushSilentImpl
{
private:
  const bool previouslySilent
#ifndef RELEASE_BUILD
    , previouslyBreaking
#endif
    ;

public:
  lsErrorPushSilentImpl() :
    previouslySilent(_ls_error_silent)
#ifndef RELEASE_BUILD
    ,
    previouslyBreaking(_ls_error_break)
#endif
  {
    if (!previouslySilent)
      _ls_error_silent = true;

#ifndef RELEASE_BUILD
    if (previouslyBreaking)
      _ls_error_break = false;
#endif
  };

  ~lsErrorPushSilentImpl()
  {
    if (!previouslySilent)
      _ls_error_silent = false;

#ifndef RELEASE_BUILD
    if (previouslyBreaking)
      _ls_error_break = true;
#endif
  }
};

#define LS_SILENCE_ERROR(result) (lsErrorPushSilentImpl(), result)

#ifdef _DEBUG
#define LS_SILENCE_ERROR_DEBUG(result) mSILENCE_ERROR(result)
#else
#define LS_SILENCE_ERROR_DEBUG(result) (result)
#endif

#define LS_SILENT_ERROR_SET(result) \
  do { lsErrorPushSilentImpl __silence__##__LINE__##_; LS_ERROR_SET(result); } while (0)

#define LS_SILENT_ERROR_CHECK(functionCall) \
  do { result = functionCall; { lsErrorPushSilentImpl __silence__##__LINE__##_; LS_ERROR_CHECK(result); } } while (0)

#define LS_SILENT_ERROR_IF(conditional, resultOnError) \
  do { lsErrorPushSilentImpl __silence__##__LINE__##_; LS_ERROR_IF(conditional, resultOnError); } while (0)


#define LS_ARRAYSIZE_C_STYLE(arrayName) (sizeof(arrayName) / sizeof(arrayName[0]))

#ifdef LS_FORCE_ARRAYSIZE_C_STYLE
#define LS_ARRAYSIZE(arrayName) LS_ARRAYSIZE_C_STYLE(arrayName)
#else
template <typename T, size_t TCount>
inline constexpr size_t LS_ARRAYSIZE(const T(&)[TCount]) { return TCount; }
#endif

#ifdef _DEBUG
#define lsFail() \
  do \
  { puts("Invalid Path Taken in File '" __FILE__ "' (Line " LS_STRINGIFY_VALUE(__LINE__) ")"); \
    __debugbreak(); \
  } while (0)

#define lsAssertInternal(a, expression_text) \
  do \
  { if (!(a)) \
    { const char *__func_name = __FUNCTION__; \
      constexpr size_t __func_size = LS_ARRAYSIZE(__FUNCTION__); \
      constexpr size_t __assert_text_size = LS_ARRAYSIZE(": Assertion Failed ('" expression_text "') in File '" __FILE__ "' (Line " LS_STRINGIFY_VALUE(__LINE__) ")");\
      char __output_text[__assert_text_size + __func_size] = ": Assertion Failed ('" expression_text "') in File '" __FILE__ "' (Line " LS_STRINGIFY_VALUE(__LINE__) ")"; \
      memmove(__output_text + __func_size - 1, __output_text, __assert_text_size); \
      memcpy(__output_text, __func_name, __func_size - 1); \
      if (lsPrintErrorCallback == nullptr) puts(__output_text); \
      else lsPrintErrorCallback(__output_text); \
      __debugbreak(); \
  } } while (0)

#define lsAssert(a) lsAssertInternal(a, #a)
#else
#define lsFail() do { } while (0)
#define lsAssertInternal(a, expression_text) do { if (!(a)) { } } while (0)
#define lsAssert(a) do { if (!(a)) { } } while (0)
#endif

#define LS_DEBUG_ERROR_ASSERT(a) \
  do \
  { const lsResult __temp_result = (a); \
    (void)__temp_result; \
    lsAssertInternal(LS_SUCCESS(__temp_result), "LS_SUCCESS(" #a ")"); \
  } while (0)

#define LS_DEBUG_ASSERT_TRUE(a) \
  do \
  { const bool __temp_result = !!(a); \
    (void)__temp_result; \
    lsAssertInternal(__temp_result, #a); \
  } while (0)

template <typename T, typename U>
constexpr inline auto lsMax(const T &a, const U &b) -> decltype(a > b ? a : b)
{
  return a > b ? a : b;
}

template <typename T, typename U>
constexpr inline auto lsMin(const T &a, const U &b) -> decltype(a < b ? a : b)
{
  return a < b ? a : b;
}

template <typename T>
constexpr inline T lsClamp(const T &a, const T &min, const T &max)
{
  if (a < min)
    return min;

  if (a > max)
    return max;

  return a;
}

//////////////////////////////////////////////////////////////////////////

enum lsConsoleColor
{
  lsCC_Black,
  lsCC_DarkRed,
  lsCC_DarkGreen,
  lsCC_DarkYellow,
  lsCC_DarkBlue,
  lsCC_DarkMagenta,
  lsCC_DarkCyan,
  lsCC_BrightGray,
  lsCC_DarkGray,
  lsCC_BrightRed,
  lsCC_BrightGreen,
  lsCC_BrightYellow,
  lsCC_BrightBlue,
  lsCC_BrightMagenta,
  lsCC_BrightCyan,
  lsCC_White
};

void lsCreateConsole();
void lsResetConsoleColor();
void lsSetConsoleColor(const lsConsoleColor foreground, const lsConsoleColor background);

typedef void lsPrintCallbackFunc(const char *);
extern lsPrintCallbackFunc *lsPrintCallback;
extern lsPrintCallbackFunc *lsPrintErrorCallback;
extern lsPrintCallbackFunc *lsPrintLogCallback;

void lsDefaultPrint(const char *text);
void lsDefaultPrintError(const char *text);
void lsDefaultPrintLog(const char *text);

//////////////////////////////////////////////////////////////////////////

template <typename T>
inline void lsZeroMemory(_Out_ T *pData, size_t count = 1)
{
  if (pData == nullptr)
    return;

  memset(reinterpret_cast<void *>(pData), 0, sizeof(T) * count);
}

template <typename T>
inline void lsMemset(_Out_ T *pData, const size_t count, uint8_t data = 0)
{
  if (pData == nullptr)
    return;

  memset(reinterpret_cast<void *>(pData), (int)data, sizeof(T) * count);
}

template <typename T>
inline void lsMemcpy(T *pDst, const T *pSrc, const size_t count)
{
  lsAssert((pDst < pSrc && pDst + count <= pSrc) || (pSrc < pDst && pSrc + count <= pDst)); // use memmove instead!
  memcpy(pDst, pSrc, sizeof(T) * count);
}

template <typename T>
inline void lsMemmove(T *pDst, const T *pSrc, const size_t count)
{
  memmove(pDst, pSrc, sizeof(T) * count);
}

template <typename T>
inline lsResult lsAlloc(_Out_ T **ppData, const size_t count = 1)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(ppData == nullptr, lsR_ArgumentNull);

  // Allocate Memory.
  {
    const size_t size = sizeof(T) * count;
    T *pData = reinterpret_cast<T *>(malloc(size));
    LS_ERROR_IF(pData == nullptr, lsR_MemoryAllocationFailure);
    *ppData = pData;
  }

epilogue:
  if (LS_FAILED(result))
    *ppData = nullptr;

  return result;
}

template <typename T>
inline lsResult lsAllocZero(_Out_ T **ppData, const size_t count = 1)
{
  lsResult result = lsR_Success;

  LS_ERROR_CHECK(lsAlloc(ppData, count));
  lsZeroMemory(*ppData, count);

epilogue:
  return result;
}

template <typename T>
inline lsResult lsRealloc(T **ppData, const size_t newCount)
{
  lsResult result = lsR_Success;
  T *pData;

  LS_ERROR_IF(ppData == nullptr, lsR_ArgumentNull);

  pData = reinterpret_cast<T *>(realloc(*ppData, sizeof(T) * newCount));
  LS_ERROR_IF(pData == nullptr, lsR_MemoryAllocationFailure);

  *ppData = pData;

  goto epilogue;

epilogue:
  if (LS_FAILED(result))
    *ppData = nullptr;

  return result;
}

template <typename T>
inline void lsFreePtr(_In_Out_ T **ppData)
{
  if (ppData != nullptr && *ppData != nullptr)
  {
    free((void *)(*ppData));
    *ppData = nullptr;
  }
}

//////////////////////////////////////////////////////////////////////////

constexpr double_t lsPI = M_PI;
constexpr double_t lsTWOPI = 2 * lsPI;
constexpr double_t lsHALFPI = M_PI_2;
constexpr double_t lsQUARTERPI = M_PI_4;
constexpr double_t lsSQRT2 = M_SQRT2;
constexpr double_t lsINVSQRT2 = M_SQRT1_2;
constexpr double_t lsSQRT3 = 1.73205080756887729352744634150587236694;
constexpr double_t lsINV_SQRT3 = 0.5773502691896257645091487805019574556;
constexpr double_t lsSQRT5 = 2.23606797749978969640917366873127623544;
constexpr double_t lsINV_SQRT5 = 0.4472135954999579392818347337462552471;

constexpr float_t lsPIf = 3.141592653589793f;
constexpr float_t lsTWOPIf = 6.283185307179586f;
constexpr float_t lsHALFPIf = ((float_t)M_PI_2);
constexpr float_t lsQUARTERPIf = ((float_t)M_PI_4);
constexpr float_t lsSQRT2f = 1.414213562373095f;
constexpr float_t lsINVSQRT2f = 0.7071067811865475f;
constexpr float_t lsSQRT3f = 1.732050807568877f;
constexpr float_t lsINVSQRT3f = 0.57735026918962576f;

constexpr double_t lsEULER = M_E;
constexpr double_t lsLOG2E = M_LOG2E;
constexpr double_t lsLOG10E = M_LOG10E;
constexpr double_t lsLN2 = M_LN2;
constexpr double_t lsLN10 = M_LN10;
constexpr double_t lsPHI = 1.6180339887498948482045868; // golden ratio.

constexpr float_t lsEULERf = ((float_t)M_E);
constexpr float_t lsLOG2Ef = ((float_t)M_LOG2E);
constexpr float_t lsLOG10Ef = ((float_t)M_LOG10E);
constexpr float_t lsLN2f = ((float_t)M_LN2);
constexpr float_t lsLN10f = ((float_t)M_LN10);
constexpr float_t lsPHIf = 1.6180339887498948482045868f; // golden ratio.

constexpr double_t lsDEG2RAD = (lsPI / 180.0);
constexpr float_t lsDEG2RADf = (lsPIf / 180.0f);

constexpr double_t lsRAD2DEG = (180.0 / lsPI);
constexpr float_t lsRAD2DEGf = (180.0f / lsPIf);

//////////////////////////////////////////////////////////////////////////

template <typename T> constexpr inline T lsAbs(const T value) { return value >= 0 ? value : -value; }
template <> inline float_t lsAbs(const float_t value) { return fabsf(value); }
template <> inline double_t lsAbs(const double_t value) { return abs(value); }

inline double_t lsFloor(const double_t value) { return floor(value); }
inline float_t lsFloor(const float_t value) { return floorf(value); }
inline double_t lsCeil(const double_t value) { return ceil(value); }
inline float_t lsCeil(const float_t value) { return ceilf(value); }
inline double_t lsRound(const double_t value) { return round(value); }
inline float_t lsRound(const float_t value) { return roundf(value); }
inline double_t lsCopySign(const double_t value, const double_t sign) { return copysign(value, sign); }
inline float_t lsCopySign(const float_t value, const float_t sign) { return copysignf(value, sign); }

inline double_t lsFractionalPart(const double_t value, double_t *pIntegralPart) { return modf(value, pIntegralPart); }
inline float_t lsFractionalPart(const float_t value, float_t *pIntegralPart) { return modff(value, pIntegralPart); }

inline double_t lsFractionalPart(const double_t value) { double_t _unused; return modf(value, &_unused); }
inline float_t lsFractionalPart(const float_t value) { float_t _unused; return modff(value, &_unused); }

template <typename T> constexpr inline T lsMax(const T a, const T b) { return (a >= b) ? a : b; }
template <typename T> constexpr inline T lsMin(const T a, const T b) { return (a <= b) ? a : b; }

template <typename T, typename U>
constexpr inline auto lsLerp(const T a, const T b, const U ratio) -> decltype(a + (b - a) * ratio) { return a + (b - a) * ratio; }

template <typename T, typename U = typename std::conditional_t<std::is_integral<T>::value, float_t, T>>
constexpr inline U lsInverseLerp(const T value, const T min, const T max) { return (U)(value - min) / (U)(max - min); }

template <typename T, typename U>
constexpr inline auto lsBiLerp(const T a, const T b, const T c, const T d, const U ratio1, const U ratio2) -> decltype(lsLerp(lsLerp(a, b, ratio1), lsLerp(c, d, ratio1), ratio2)) { return lsLerp(lsLerp(a, b, ratio1), lsLerp(c, d, ratio1), ratio2); }

// Indices are: Z, Y, X.
template <typename T, typename U>
constexpr inline T lsTriLerp(const T v000, const T v001, const T v010, const T v011, const T v100, const T v101, const T v110, const T v111, const U factor_001, const U factor_010, const U factor_100)
{
  const U inverseFactor_001 = (U)1 - factor_001;
  const U inverseFactor_010 = (U)1 - factor_010;
  const U inverseFactor_100 = (U)1 - factor_100;

  const T c00 = v000 * inverseFactor_100 + v100 * factor_100;
  const T c01 = v001 * inverseFactor_100 + v101 * factor_100;
  const T c10 = v010 * inverseFactor_100 + v110 * factor_100;
  const T c11 = v011 * inverseFactor_100 + v111 * factor_100;

  const T c0 = c00 * inverseFactor_010 + c10 * factor_010;
  const T c1 = c01 * inverseFactor_010 + c11 * factor_010;

  return c0 * inverseFactor_001 + c1 * factor_001;
}

template <typename T, typename U = typename std::conditional_t<std::is_integral<T>::value, float_t, T>>
constexpr inline U lsSmoothStep(const T x)
{
  const U ux = (U)x;

  return lsClamp(((U)3 - ux * (U)2) * ux * ux, (U)0, (U)1);
}

template <typename T, typename U = typename std::conditional_t<std::is_integral<T>::value, float_t, T>>
constexpr inline U lsSmoothStepUnclamped(const T x)
{
  const U ux = (U)x;

  return ((U)3 - ux * (U)2) * ux * ux;
}

template <typename T, typename U = typename std::conditional_t<std::is_integral<T>::value, float_t, T>>
constexpr inline U lsSmootherStep(const T x)
{
  const U ux = (U)x;

  return lsClamp(ux * ux * ux * (ux * (ux * (U)6 - (U)15) + (U)10), (U)0, (U)1);
}

template <typename T, typename U = typename std::conditional_t<std::is_integral<T>::value, float_t, T>>
constexpr inline U lsSmootherStepUnclamped(const T x)
{
  const U ux = (U)x;

  return ux * ux * ux * (ux * (ux * (U)6 - (U)15) + (U)10);
}

// `y0` is the value at relative -1.
// `y1` is the value at relative 0.
// `y2` is the value at relative 1.
// `y3` is the value at relative 2.
// `x` is the interpolated position.
template <typename T, typename U>
constexpr inline T lsInterpolateCubic(const T y0, const T y1, const T y2, const T y3, const U x) { return y1 + ((y2 - y0 + ((U)2 * y0 - (U)5 * y1 + (U)4 * y2 - y3 + ((U)3 * (y1 - y2) + y3 - y0) * x) * x) * x) / (U)2; }

template <typename T, typename U>
constexpr inline T lsInterpolateBicubic(const T data[4][4], const U x, const U y)
{
  T values[4];

  for (size_t i = 0; i < 4; i++)
    values[i] = lsInterpolateCubic(data[i][0], data[i][1], data[i][2], data[i][3], y);

  return lsInterpolateCubic(values[0], values[1], values[2], values[3], x);
}

template <typename T, typename U>
struct lsInterpolateBicubic_t
{
  T values[4];

  lsInterpolateBicubic_t(const T data[4][4], const U y)
  {
    for (size_t i = 0; i < 4; i++)
      values[i] = lsInterpolateCubic(data[i][0], data[i][1], data[i][2], data[i][3], y);
  }

  T SampleAt(const U x)
  {
    return lsInterpolateCubic(values[0], values[1], values[2], values[3], x);
  }
};

template <typename T, typename U>
constexpr inline T lsInterpolateTricubic(const T data[4][4][4], const U x, const U y, const U z)
{
  T values[4];

  for (size_t i = 0; i < 4; i++)
    values[i] = lsInterpolateBicubic(data[i][0], data[i][1], data[i][2], data[i][3], y, z);

  return lsInterpolateCubic(values[0], values[1], values[2], values[3], x);
}

template <typename T, typename std::enable_if_t<!std::is_unsigned<T>::value, int> * = nullptr> constexpr inline T lsSign(const T value) { return value > 0 ? (T)1 : (value < 0 ? (T)-1 : (T)0); }
template <typename T, typename std::enable_if_t<std::is_unsigned<T>::value, int> * = nullptr> constexpr inline T lsSign(const T value) { return value > 0 ? (T)1 : (T)0; }

template <typename T> inline auto lsSqrt(const T value) -> typename std::enable_if<!std::is_same<T, float_t>::value, double_t>::type { return sqrt((double_t)value); }
template <typename T> inline auto lsSqrt(const T value) -> typename std::enable_if< std::is_same<T, float_t>::value, float_t >::type { return sqrtf(value); }
template <typename T> inline auto lsSin(const T value) -> typename std::enable_if<!std::is_same<T, float_t>::value, double_t>::type { return sin((double_t)value); }
template <typename T> inline auto lsSin(const T value) -> typename std::enable_if< std::is_same<T, float_t>::value, float_t >::type { return sinf(value); }
template <typename T> inline auto lsCos(const T value) -> typename std::enable_if<!std::is_same<T, float_t>::value, double_t>::type { return cos((double_t)value); }
template <typename T> inline auto lsCos(const T value) -> typename std::enable_if< std::is_same<T, float_t>::value, float_t >::type { return cosf(value); }
template <typename T> inline auto lsTan(const T value) -> typename std::enable_if<!std::is_same<T, float_t>::value, double_t>::type { return tan((double_t)value); }
template <typename T> inline auto lsTan(const T value) -> typename std::enable_if< std::is_same<T, float_t>::value, float_t >::type { return tanf(value); }
template <typename T> inline auto lsASin(const T value) -> typename std::enable_if<!std::is_same<T, float_t>::value, double_t>::type { return asin((double_t)value); }
template <typename T> inline auto lsASin(const T value) -> typename std::enable_if< std::is_same<T, float_t>::value, float_t >::type { return asinf(value); }
template <typename T> inline auto lsACos(const T value) -> typename std::enable_if<!std::is_same<T, float_t>::value, double_t>::type { return acos((double_t)value); }
template <typename T> inline auto lsACos(const T value) -> typename std::enable_if< std::is_same<T, float_t>::value, float_t >::type { return acosf(value); }
template <typename T> inline auto lsATan(const T value) -> typename std::enable_if<!std::is_same<T, float_t>::value, double_t>::type { return atan((double_t)value); }
template <typename T> inline auto lsATan(const T value) -> typename std::enable_if< std::is_same<T, float_t>::value, float_t >::type { return atanf(value); }
template <typename T, typename U> inline auto lsATan2(const T value, const U value2) -> typename std::enable_if<!std::is_same<T, float_t>::value, double_t>::type { return atan2((double_t)value, (double_t)value2); }
template <typename T, typename U> inline auto lsATan2(const T value, const U value2) -> typename std::enable_if< std::is_same<T, float_t>::value, float_t >::type { return atan2f(value, value2); }

template <typename U>
inline auto lsPow(const float_t value, const U value2) -> typename std::enable_if_t<!std::is_same<U, double_t>::value, float_t>
{
  return powf(value, (float_t)value2);
}

template <typename T, typename U>
inline auto lsPow(const T value, const U value2) -> typename std::enable_if_t<!std::is_same<U, float_t>::value || !std::is_same<T, float_t>::value, decltype(pow(value, value2))>
{
  return pow(value, value2);
}

template <typename T>
struct lsFloatTypeFrom_Internal
{
  typedef double_t type;
};

template <>
struct lsFloatTypeFrom_Internal<float_t>
{
  typedef float_t type;
};

template <typename T>
using lsFloatTypeFrom = typename lsFloatTypeFrom_Internal<T>::type;

template <typename T> constexpr inline auto lsLog(const T value) -> lsFloatTypeFrom<T> { return (lsFloatTypeFrom<T>)log((lsFloatTypeFrom<T>)value); }
template <typename T> constexpr inline auto lsLog10(const T value) -> lsFloatTypeFrom<T> { return (lsFloatTypeFrom<T>)log10((lsFloatTypeFrom<T>)value); }
template <typename T> constexpr inline auto lsLog2(const T value) -> lsFloatTypeFrom<T> { return (lsFloatTypeFrom<T>)log2((lsFloatTypeFrom<T>)value); }
template <typename T> constexpr inline auto lsLogN(const T logarithm, const T value) -> lsFloatTypeFrom<T> { return lsLog(value) / lsLog(logarithm); }

template <typename T>
T lsMod(const T value, const T modulus);

template <>
uint64_t constexpr lsMod<uint64_t>(const uint64_t value, const uint64_t modulus)
{
  return value % modulus;
}

template <>
int64_t constexpr lsMod<int64_t>(const int64_t value, const int64_t modulus)
{
  return value % modulus;
}

template <>
uint32_t constexpr lsMod<uint32_t>(const uint32_t value, const uint32_t modulus)
{
  return value % modulus;
}

template <>
int32_t constexpr lsMod<int32_t>(const int32_t value, const int32_t modulus)
{
  return value % modulus;
}

template <>
uint16_t constexpr lsMod<uint16_t>(const uint16_t value, const uint16_t modulus)
{
  return value % modulus;
}

template <>
int16_t constexpr lsMod<int16_t>(const int16_t value, const int16_t modulus)
{
  return value % modulus;
}

template <>
uint8_t constexpr lsMod<uint8_t>(const uint8_t value, const uint8_t modulus)
{
  return value % modulus;
}

template <>
int8_t constexpr lsMod<int8_t>(const int8_t value, const int8_t modulus)
{
  return value % modulus;
}

template <>
inline float_t lsMod<float_t>(const float_t value, const float_t modulus)
{
  return fmodf(value, modulus);
}

template <>
inline double_t lsMod<double_t>(const double_t value, const double_t modulus)
{
  return fmod(value, modulus);
}

// Euclidean modulo. (For positive modulus).
template <typename T>
constexpr inline T lsEuclideanMod(const T value, const T modulus)
{
  const T v = lsMod(value, modulus);
  return v < (T)0 ? (v + modulus) : v;
};

// Returns 1 if equal, 0 if opposite, anything in between is an indicator for *how* similar they are.
template <typename T>
inline constexpr T lsAngleCompare(const T a, const T b)
{
  return lsAbs(lsEuclideanMod(a - b, (T)lsTWOPI) - (T)lsPI) / (T)lsPIf;
}

//////////////////////////////////////////////////////////////////////////

inline constexpr uint64_t lsLowestBit(const uint64_t value)
{
#ifndef BIT_FALLBACK
  return std::countr_zero(value);
#else
#ifdef _MSC_VER
  unsigned long bit;
  _BitScanForward64(&bit, value);
#else
  const uint64_t bit = __builtin_ctzll(value);
#endif

  return bit;
#endif
}

inline constexpr uint32_t lsLowestBit(const uint32_t value)
{
#ifndef BIT_FALLBACK
  return std::countr_zero(value);
#else
#ifdef _MSC_VER
  unsigned long bit;
  _BitScanForward(&bit, value);
#else
  const uint32_t bit = __builtin_ctz(value);
#endif

  return bit;
#endif
}

inline constexpr uint64_t lsHighestBit(const uint64_t value)
{
#ifndef BIT_FALLBACK
  return 63 - std::countl_zero(value);
#else
#ifdef _MSC_VER
  unsigned long bit;
  _BitScanReverse64(&bit, value);
#else
  const uint64_t bit = sizeof(value) * CHAR_BIT - 1 - __builtin_clzll(value);
#endif

  return bit;
#endif
}

inline constexpr uint32_t lsHighestBit(const uint32_t value)
{
#ifndef BIT_FALLBACK
  return 31 - std::countl_zero(value);
#else
#ifdef _MSC_VER
  unsigned long bit;
  _BitScanReverse(&bit, value);
#else
  const uint32_t bit = sizeof(value) * CHAR_BIT - 1 - __builtin_clz(value);
#endif

  return bit;
#endif
}

template <typename T>
  requires (std::is_integral_v<T> &&std::is_unsigned_v<T>)
inline constexpr T lsBitCeil(const T x)
{
  if (x <= 1)
    return 1;
  else
    return (T)1 << (1 + lsHighestBit(x - 1));
}

template <typename T>
  requires (std::is_integral_v<T> &&std::is_unsigned_v<T>)
inline constexpr T lsBitFloor(const T x)
{
  if (x == 0)
    return 0;
  else
    return (T)1 << (lsHighestBit(x));
}

//////////////////////////////////////////////////////////////////////////

template <typename TKey>
typename std::enable_if<(std::is_integral<TKey>::value || std::is_enum<TKey>::value) && sizeof(TKey) <= sizeof(uint32_t), uint32_t>::type hash(const TKey value)
{
  const uint64_t mul = value * 6364136223846793005ULL;
  const uint32_t xorshifted = (uint32_t)(((mul >> 18) ^ mul) >> 27);
  const uint32_t rot = (uint32_t)(mul >> 59);

  return (xorshifted >> rot) | (xorshifted << (uint32_t)((-(int32_t)rot) & 31));
}

template <typename TKey>
typename std::enable_if<(std::is_integral<TKey>::value || std::is_enum<TKey>::value) && sizeof(TKey) == sizeof(uint64_t), uint32_t>::type hash(const TKey value)
{
  const uint32_t a = hash((uint32_t)value);
  const uint32_t b = hash((uint32_t)(value >> 32));
  return a ^ b;
}

//////////////////////////////////////////////////////////////////////////

#define _VECTOR_SUBSET_2(a, b) constexpr inline vec2t<T> a ## b() const { return vec2t<T>(a, b); }
#define _VECTOR_SUBSET_3(a, b, c) constexpr inline vec3t<T> a ## b ## c() const { return vec3t<T>(a, b, c); }
#define _VECTOR_SUBSET_4(a, b, c, d) constexpr inline vec4t<T> a ## b ## c ## d() const { return vec4t<T>(a, b, c, d); }

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct vec2t
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)
#endif
  union
  {
    T asArray[2];

    struct
    {
      T x, y;
    };
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

  inline constexpr vec2t() : x(0), y(0) {}
  inline constexpr explicit vec2t(T _v) : x(_v), y(_v) {}

  // Cartesian: x, y;
  // Polar: r, theta;
  inline constexpr vec2t(T _x, T _y) : x(_x), y(_y) {}

  template <typename T2> inline constexpr explicit vec2t(const vec2t<T2> &cast) : x((T)cast.x), y((T)cast.y) { }

  inline vec2t<T>  operator +  (const vec2t<T> &a) const { return vec2t<T>(x + a.x, y + a.y); };
  inline vec2t<T>  operator -  (const vec2t<T> &a) const { return vec2t<T>(x - a.x, y - a.y); };
  inline vec2t<T>  operator *  (const vec2t<T> &a) const { return vec2t<T>(x * a.x, y * a.y); };
  inline vec2t<T>  operator /  (const vec2t<T> &a) const { return vec2t<T>(x / a.x, y / a.y); };
  inline vec2t<T> &operator += (const vec2t<T> &a) { return *this = vec2t<T>(x + a.x, y + a.y); };
  inline vec2t<T> &operator -= (const vec2t<T> &a) { return *this = vec2t<T>(x - a.x, y - a.y); };
  inline vec2t<T> &operator *= (const vec2t<T> &a) { return *this = vec2t<T>(x * a.x, y * a.y); };
  inline vec2t<T> &operator /= (const vec2t<T> &a) { return *this = vec2t<T>(x / a.x, y / a.y); };
  inline vec2t<T>  operator *  (const T a) const { return vec2t<T>(x * a, y * a); };
  inline vec2t<T>  operator /  (const T a) const { return vec2t<T>(x / a, y / a); };
  inline vec2t<T> &operator *= (const T a) { return *this = vec2t<T>(x * a, y * a); };
  inline vec2t<T> &operator /= (const T a) { return *this = vec2t<T>(x / a, y / a); };
  inline vec2t<T>  operator << (const T a) const { return vec2t<T>(x << a, y << a); };
  inline vec2t<T>  operator >> (const T a) const { return vec2t<T>(x >> a, y >> a); };
  inline vec2t<T> &operator <<= (const T a) { return *this = vec2t<T>(x << a, y << a); };
  inline vec2t<T> &operator >>= (const T a) { return *this = vec2t<T>(x >> a, y >> a); };

  inline vec2t<T>  operator -  () const { return vec2t<T>(-x, -y); };

  inline bool      operator == (const vec2t<T> &a) const { return x == a.x && y == a.y; };
  inline bool      operator != (const vec2t<T> &a) const { return x != a.x || y != a.y; };

  inline lsFloatTypeFrom<T> Length() const { return lsSqrt(x * x + y * y); };
  inline T LengthSquared() const { return x * x + y * y; };
  inline vec2t<T> Normalize() const { return *this / (T)Length(); };

  inline float_t AspectRatio() const { return (float_t)x / (float_t)y; };
  inline float_t InverseAspectRatio() const { return (float_t)y / (float_t)x; };

  inline static T Dot(const vec2t<T> a, const vec2t<T> b)
  {
    return a.x * b.x + a.y * b.y;
  };

  inline vec2t<T> CartesianToPolar()
  {
    return vec2t<T>(Length(), lsATan2(y, x));
  };

  inline vec2t<T> PolarToCartesian()
  {
    return vec2t<T>(x * lsCos(y), x * lsSin(y));
  };

  _VECTOR_SUBSET_2(y, x);
};

template <typename T>
inline vec2t<T>  operator *  (const T a, const vec2t<T> b) { return vec2t<T>(a * b.x, a * b.y); };

template <typename T>
inline vec2t<T>  operator /  (const T a, const vec2t<T> b) { return vec2t<T>(a / b.x, a / b.y); };

template <typename T> T lsMax(const vec2t<T> &v) { return lsMax(v.x, v.y); }
template <typename T> T lsMin(const vec2t<T> &v) { return lsMin(v.x, v.y); }

template <typename T> vec2t<T> lsMax(const vec2t<T> &a, const vec2t<T> &b) { return vec2t<T>(lsMax(a.x, b.x), lsMax(a.y, b.y)); }
template <typename T> vec2t<T> lsMin(const vec2t<T> &a, const vec2t<T> &b) { return vec2t<T>(lsMin(a.x, b.x), lsMin(a.y, b.y)); }

template <typename T> vec2t<T> lsAbs(const vec2t<T> &a) { return vec2t<T>(lsAbs(a.x), lsAbs(a.y)); }
template <typename T> vec2t<T> lsFloor(const vec2t<T> &a) { return vec2t<T>(lsFloor(a.x), lsFloor(a.y)); }
template <typename T> vec2t<T> lsCeil(const vec2t<T> &a) { return vec2t<T>(lsCeil(a.x), lsCeil(a.y)); }
template <typename T> vec2t<T> lsRound(const vec2t<T> &a) { return vec2t<T>(lsRound(a.x), lsRound(a.y)); }

typedef vec2t<size_t> vec2s;
typedef vec2t<int64_t> vec2i;
typedef vec2t<uint64_t> vec2u;
typedef vec2t<int32_t> vec2i32;
typedef vec2t<int16_t> vec2i16;
typedef vec2t<int8_t> vec2i8;
typedef vec2t<uint32_t> vec2u32;
typedef vec2t<uint16_t> vec2u16;
typedef vec2t<uint8_t> vec2u8;
typedef vec2t<uint32_t> vec2u32;
typedef vec2t<float_t> vec2f;
typedef vec2t<double_t> vec2d;

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct vec3t
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)
#endif
  union
  {
    T asArray[3];

    struct
    {
      T x, y, z;
    };
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

  constexpr inline vec3t() : x(0), y(0), z(0) {}
  constexpr inline explicit vec3t(T _v) : x(_v), y(_v), z(_v) {}

  // Cartesian: x, y, z;
  // Spherical: radius, theta, phi;
  // Cylindrical: rho, phi, z;
  constexpr inline vec3t(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
  constexpr inline explicit vec3t(vec2t<T> vector2, T _z) : x(vector2.x), y(vector2.y), z(_z) {}
  constexpr inline explicit vec3t(T _x, vec2t<T> vector2) : x(_x), y(vector2.x), z(vector2.y) {}

  template <typename T2> constexpr inline explicit vec3t(const vec3t<T2> &cast) : x((T)cast.x), y((T)cast.y), z((T)cast.z) {}

  constexpr inline vec3t<T>  operator +  (const vec3t<T> &a) const { return vec3t<T>(x + a.x, y + a.y, z + a.z); };
  constexpr inline vec3t<T>  operator -  (const vec3t<T> &a) const { return vec3t<T>(x - a.x, y - a.y, z - a.z); };
  constexpr inline vec3t<T>  operator *  (const vec3t<T> &a) const { return vec3t<T>(x * a.x, y * a.y, z * a.z); };
  constexpr inline vec3t<T>  operator /  (const vec3t<T> &a) const { return vec3t<T>(x / a.x, y / a.y, z / a.z); };
  constexpr inline vec3t<T> &operator += (const vec3t<T> &a) { return *this = vec3t<T>(x + a.x, y + a.y, z + a.z); };
  constexpr inline vec3t<T> &operator -= (const vec3t<T> &a) { return *this = vec3t<T>(x - a.x, y - a.y, z - a.z); };
  constexpr inline vec3t<T> &operator *= (const vec3t<T> &a) { return *this = vec3t<T>(x * a.x, y * a.y, z * a.z); };
  constexpr inline vec3t<T> &operator /= (const vec3t<T> &a) { return *this = vec3t<T>(x / a.x, y / a.y, z / a.z); };
  constexpr inline vec3t<T>  operator *  (const T a) const { return vec3t<T>(x * a, y * a, z * a); };
  constexpr inline vec3t<T>  operator /  (const T a) const { return vec3t<T>(x / a, y / a, z / a); };
  constexpr inline vec3t<T> &operator *= (const T a) { return *this = vec3t<T>(x * a, y * a, z * a); };
  constexpr inline vec3t<T> &operator /= (const T a) { return *this = vec3t<T>(x / a, y / a, z / a); };

  constexpr inline vec3t<T>  operator -  () const { return vec3t<T>(-x, -y, -z); };

  constexpr inline bool      operator == (const vec3t<T> &a) const { return x == a.x && y == a.y && z == a.z; };
  constexpr inline bool      operator != (const vec3t<T> &a) const { return x != a.x || y != a.y || z != a.z; };

  inline lsFloatTypeFrom<T> Length() const { return lsSqrt(x * x + y * y + z * z); };
  constexpr inline T LengthSquared() const { return x * x + y * y + z * z; };
  inline vec3t<T> Normalize() const { return *this / (T)Length(); };

  constexpr inline vec2t<T> ToVector2() const { return vec2t<T>(x, y); };

  constexpr inline static vec3t<T> Cross(const vec3t<T> a, const vec3t<T> b)
  {
    return vec3t<T>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
  };

  constexpr inline static T Dot(const vec3t<T> a, const vec3t<T> b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  };

  inline vec3t<T> CartesianToSpherical() const
  {
    const auto r = Length();

    return vec3t<T>((T)r, (T)lsATan2(y, x), (T)lsATan2(lsSqrt(x * x + y * y), z));
  };

  inline vec3t<T> SphericalToCartesian() const
  {
    const auto sinTheta = lsSin(y);

    return vec3t<T>(x * sinTheta * lsCos(z), x * sinTheta * lsSin(z), x * lsCos(y));
  };

  inline vec3t<T> SphericalToCylindrical() const
  {
    return vec3t<T>(x * lsSin(y), z, x * lsCos(y));
  };

  inline vec3t<T> CylindricalToSpherical() const
  {
    const auto r = lsSqrt(x * x + z * z);

    return vec3t<T>((T)r, y, (T)lsACos(z / r));
  };

  inline vec3t<T> CylindricalToCartesian() const
  {
    return vec3t<T>(x * lsCos(y), x * lsSin(y), z);
  };

  inline vec3t<T> CartesianToCylindrical() const
  {
    const auto p = lsSqrt(x * x + y * y);

    if (x == 0 && y == 0)
      return vec3t<T>(p, 0, z);
    else if (x >= 0)
      return vec3t<T>(p, (T)lsASin(y / p), z);
    else
      return vec3t<T>(p, (T)(-lsASin(y / p) + lsPI), z);
  };

  _VECTOR_SUBSET_2(x, y);
  _VECTOR_SUBSET_2(x, z);
  _VECTOR_SUBSET_2(y, x);
  _VECTOR_SUBSET_2(y, z);
  _VECTOR_SUBSET_2(z, x);
  _VECTOR_SUBSET_2(z, y);

  _VECTOR_SUBSET_3(x, z, y);
  _VECTOR_SUBSET_3(y, x, z);
  _VECTOR_SUBSET_3(y, z, x);
  _VECTOR_SUBSET_3(z, x, y);
  _VECTOR_SUBSET_3(z, y, x);
};

template <typename T>
constexpr inline vec3t<T>  operator *  (const T a, const vec3t<T> b) { return vec3t<T>(a * b.x, a * b.y, a * b.z); };

template <typename T>
constexpr inline vec3t<T>  operator /  (const T a, const vec3t<T> b) { return vec3t<T>(a / b.x, a / b.y, a / b.z); };

template <typename T> constexpr inline T lsMax(const vec3t<T> &v) { return lsMax(lsMax(v.x, v.y), v.z); }
template <typename T> constexpr inline T lsMin(const vec3t<T> &v) { return lsMin(lsMin(v.x, v.y), v.z); }

template <typename T> constexpr inline vec3t<T> lsMax(const vec3t<T> &a, const vec3t<T> &b) { return vec3t<T>(lsMax(a.x, b.x), lsMax(a.y, b.y), lsMax(a.z, b.z)); }
template <typename T> constexpr inline vec3t<T> lsMin(const vec3t<T> &a, const vec3t<T> &b) { return vec3t<T>(lsMin(a.x, b.x), lsMin(a.y, b.y), lsMin(a.z, b.z)); }

template <typename T> inline vec3t<T> lsAbs(const vec3t<T> &a) { return vec3t<T>(lsAbs(a.x), lsAbs(a.y), lsAbs(a.z)); }
template <typename T> inline vec3t<T> lsFloor(const vec3t<T> &a) { return vec3t<T>(lsFloor(a.x), lsFloor(a.y), lsFloor(a.z)); }
template <typename T> inline vec3t<T> lsCeil(const vec3t<T> &a) { return vec3t<T>(lsCeil(a.x), lsCeil(a.y), lsCeil(a.z)); }
template <typename T> inline vec3t<T> lsRound(const vec3t<T> &a) { return vec3t<T>(lsRound(a.x), lsRound(a.y), lsRound(a.z)); }

typedef vec3t<size_t> vec3s;
typedef vec3t<int64_t> vec3i;
typedef vec3t<uint64_t> vec3u;
typedef vec3t<int32_t> vec3i32;
typedef vec3t<uint32_t> vec3u32;
typedef vec3t<float_t> vec3f;
typedef vec3t<double_t> vec3d;

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct vec4t
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)
#endif
  union
  {
    T asArray[4];

    struct
    {
      T x, y, z, w;
    };
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

  constexpr inline vec4t() : x(0), y(0), z(0), w(0) {}
  constexpr inline explicit vec4t(T _v) : x(_v), y(_v), z(_v), w(_v) {}
  constexpr inline vec4t(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
  constexpr inline explicit vec4t(const vec3t<T> vec3, const T _w) : x(vec3.x), y(vec3.y), z(vec3.z), w(_w) {}
  constexpr inline explicit vec4t(const T _x, const vec3t<T> vec3) : x(_x), y(vec3.x), z(vec3.y), w(vec3.z) {}
  constexpr inline explicit vec4t(const vec2t<T> vec2, const T _z, const T _w) : x(vec2.x), y(vec2.y), z(_z), w(_w) {}
  constexpr inline explicit vec4t(const vec2t<T> vec2a, const vec2t<T> vec2b) : x(vec2a.x), y(vec2a.y), z(vec2b.x), w(vec2b.y) {}

  template <typename T2> constexpr inline explicit vec4t(const vec4t<T2> &cast) : x((T)cast.x), y((T)cast.y), z((T)cast.z), w((T)cast.w) {}

  constexpr inline vec4t<T>  operator +  (const vec4t<T> &a) const { return vec4t<T>(x + a.x, y + a.y, z + a.z, w + a.w); };
  constexpr inline vec4t<T>  operator -  (const vec4t<T> &a) const { return vec4t<T>(x - a.x, y - a.y, z - a.z, w - a.w); };
  constexpr inline vec4t<T>  operator *  (const vec4t<T> &a) const { return vec4t<T>(x * a.x, y * a.y, z * a.z, w * a.w); };
  constexpr inline vec4t<T>  operator /  (const vec4t<T> &a) const { return vec4t<T>(x / a.x, y / a.y, z / a.z, w / a.w); };
  constexpr inline vec4t<T> &operator += (const vec4t<T> &a) { return *this = vec4t<T>(x + a.x, y + a.y, z + a.z, w + a.w); };
  constexpr inline vec4t<T> &operator -= (const vec4t<T> &a) { return *this = vec4t<T>(x - a.x, y - a.y, z - a.z, w - a.w); };
  constexpr inline vec4t<T> &operator *= (const vec4t<T> &a) { return *this = vec4t<T>(x * a.x, y * a.y, z * a.z, w * a.w); };
  constexpr inline vec4t<T> &operator /= (const vec4t<T> &a) { return *this = vec4t<T>(x / a.x, y / a.y, z / a.z, w / a.w); };
  constexpr inline vec4t<T>  operator *  (const T a) const { return vec4t<T>(x * a, y * a, z * a, w * a); };
  constexpr inline vec4t<T>  operator /  (const T a) const { return vec4t<T>(x / a, y / a, z / a, w / a); };
  constexpr inline vec4t<T> &operator *= (const T a) { return *this = vec4t<T>(x * a, y * a, z * a, w * a); };
  constexpr inline vec4t<T> &operator /= (const T a) { return *this = vec4t<T>(x / a, y / a, z / a, w / a); };

  constexpr inline vec4t<T>  operator -  () const { return vec4t<T>(-x, -y, -z, -w); };

  constexpr inline bool      operator == (const vec4t<T> &a) const { return x == a.x && y == a.y && z == a.z && w == a.w; };
  constexpr inline bool      operator != (const vec4t<T> &a) const { return x != a.x || y != a.y || z != a.z || w != a.w; };

  inline lsFloatTypeFrom<T> Length() const { return lsSqrt(x * x + y * y + z * z + w * w); };
  constexpr inline T LengthSquared() const { return x * x + y * y + z * z + w * w; };
  inline vec4t<T> Normalize() const { return *this / (T)Length(); };

  constexpr inline vec2t<T> ToVector2() const { return vec2t<T>(x, y); };
  constexpr inline vec3t<T> ToVector3() const { return vec3t<T>(x, y, z); };

  constexpr inline static T Dot(const vec4t<T> a, const vec4t<T> b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  };

  _VECTOR_SUBSET_2(x, y);
  _VECTOR_SUBSET_2(x, z);
  _VECTOR_SUBSET_2(x, w);
  _VECTOR_SUBSET_2(y, x);
  _VECTOR_SUBSET_2(y, z);
  _VECTOR_SUBSET_2(y, w);
  _VECTOR_SUBSET_2(z, x);
  _VECTOR_SUBSET_2(z, y);
  _VECTOR_SUBSET_2(z, w);
  _VECTOR_SUBSET_2(w, x);
  _VECTOR_SUBSET_2(w, y);
  _VECTOR_SUBSET_2(w, z);

  _VECTOR_SUBSET_3(x, y, z);
  _VECTOR_SUBSET_3(x, y, w);
  _VECTOR_SUBSET_3(x, z, y);
  _VECTOR_SUBSET_3(x, z, w);
  _VECTOR_SUBSET_3(x, w, y);
  _VECTOR_SUBSET_3(x, w, z);

  _VECTOR_SUBSET_3(y, x, z);
  _VECTOR_SUBSET_3(y, x, w);
  _VECTOR_SUBSET_3(y, z, x);
  _VECTOR_SUBSET_3(y, z, w);
  _VECTOR_SUBSET_3(y, w, x);
  _VECTOR_SUBSET_3(y, w, z);

  _VECTOR_SUBSET_3(z, x, y);
  _VECTOR_SUBSET_3(z, x, w);
  _VECTOR_SUBSET_3(z, y, x);
  _VECTOR_SUBSET_3(z, y, w);
  _VECTOR_SUBSET_3(z, w, x);
  _VECTOR_SUBSET_3(z, w, y);

  _VECTOR_SUBSET_4(x, y, z, w);
  _VECTOR_SUBSET_4(x, y, w, z);
  _VECTOR_SUBSET_4(x, z, y, w);
  _VECTOR_SUBSET_4(x, z, w, y);
  _VECTOR_SUBSET_4(x, w, y, z);
  _VECTOR_SUBSET_4(x, w, z, y);

  _VECTOR_SUBSET_4(y, x, z, w);
  _VECTOR_SUBSET_4(y, x, w, z);
  _VECTOR_SUBSET_4(y, z, x, w);
  _VECTOR_SUBSET_4(y, z, w, x);
  _VECTOR_SUBSET_4(y, w, x, z);
  _VECTOR_SUBSET_4(y, w, z, x);

  _VECTOR_SUBSET_4(z, x, y, w);
  _VECTOR_SUBSET_4(z, x, w, y);
  _VECTOR_SUBSET_4(z, y, x, w);
  _VECTOR_SUBSET_4(z, y, w, x);
  _VECTOR_SUBSET_4(z, w, x, y);
  _VECTOR_SUBSET_4(z, w, y, x);

  _VECTOR_SUBSET_4(w, x, y, z);
  _VECTOR_SUBSET_4(w, x, z, y);
  _VECTOR_SUBSET_4(w, y, x, z);
  _VECTOR_SUBSET_4(w, y, z, x);
  _VECTOR_SUBSET_4(w, z, x, y);
  _VECTOR_SUBSET_4(w, z, y, x);
};

template <typename T>
constexpr inline vec4t<T>  operator *  (const T a, const vec4t<T> b) { return vec4t<T>(a * b.x, a * b.y, a * b.z, a * b.w); };

template <typename T>
constexpr inline vec4t<T>  operator /  (const T a, const vec4t<T> b) { return vec4t<T>(a / b.x, a / b.y, a / b.z, a / b.w); };

template <typename T> constexpr T lsMax(const vec4t<T> &v) { return lsMax(lsMax(v.x, v.y), lsMax(v.z, v.w)); }
template <typename T> constexpr T lsMin(const vec4t<T> &v) { return lsMin(lsMin(v.x, v.y), lsMin(v.z, v.w)); }

template <typename T> constexpr vec4t<T> lsMax(const vec4t<T> &a, const vec4t<T> &b) { return vec4t<T>(lsMax(a.x, b.x), lsMax(a.y, b.y), lsMax(a.z, b.z), lsMax(a.w, b.w)); }
template <typename T> constexpr vec4t<T> lsMin(const vec4t<T> &a, const vec4t<T> &b) { return vec4t<T>(lsMin(a.x, b.x), lsMin(a.y, b.y), lsMin(a.z, b.z), lsMin(a.w, b.w)); }

template <typename T> vec4t<T> lsAbs(const vec4t<T> &a) { return vec4t<T>(lsAbs(a.x), lsAbs(a.y), lsAbs(a.z), lsAbs(a.w)); }
template <typename T> vec4t<T> lsFloor(const vec4t<T> &a) { return vec4t<T>(lsFloor(a.x), lsFloor(a.y), lsFloor(a.z), lsFloor(a.w)); }
template <typename T> vec4t<T> lsCeil(const vec4t<T> &a) { return vec4t<T>(lsCeil(a.x), lsCeil(a.y), lsCeil(a.z), lsCeil(a.w)); }
template <typename T> vec4t<T> lsRound(const vec4t<T> &a) { return vec4t<T>(lsRound(a.x), lsRound(a.y), lsRound(a.z), lsRound(a.w)); }

typedef vec4t<size_t> vec4s;
typedef vec4t<int64_t> vec4i;
typedef vec4t<uint64_t> vec4u;
typedef vec4t<int32_t> vec4i32;
typedef vec4t<uint32_t> vec4u32;
typedef vec4t<float_t> vec4f;
typedef vec4t<double_t> vec4d;

//////////////////////////////////////////////////////////////////////////

template <typename T>
inline T mSinc(const T x)
{
  return mSin((T)lsPI * x) / ((T)lsPI * x);
}

template <typename T, size_t TWidth>
inline T mLanczos(const T x)
{
  static_assert(TWidth != 0, "Invalid Parameter for TWidth.");

  if (-(T)TWidth < x && (T)TWidth > x)
    return mSinc(x) * mSinc(x / (T)TWidth);
  else if (x == 0)
    return 1;
  else
    return 0;
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
T lsMinValue();

template <typename T>
T lsMaxValue();

template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
T lsSmallest();

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
constexpr inline T lsSmallest()
{
  return (T)1;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
constexpr inline T lsSmallest(const T)
{
  return lsSmallest<T>();
}

template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
inline T lsSmallest(const T scale)
{
  return lsSmallest<T>() * lsAbs(scale);
}

template<>
constexpr int8_t lsMinValue<int8_t>()
{
  return INT8_MIN;
}

template<>
constexpr int16_t lsMinValue<int16_t>()
{
  return INT16_MIN;
}

template<>
constexpr int32_t lsMinValue<int32_t>()
{
  return INT32_MIN;
}

template<>
constexpr int64_t lsMinValue<int64_t>()
{
  return INT64_MIN;
}

template<>
constexpr uint8_t lsMinValue<uint8_t>()
{
  return 0;
}

template<>
constexpr uint16_t lsMinValue<uint16_t>()
{
  return 0;
}

template<>
constexpr uint32_t lsMinValue<uint32_t>()
{
  return 0;
}

template<>
constexpr uint64_t lsMinValue<uint64_t>()
{
  return 0;
}

template<>
constexpr float_t lsMinValue<float_t>()
{
  return -FLT_MAX;
}

template<>
constexpr double_t lsMinValue<double_t>()
{
  return -DBL_MAX;
}

template<>
constexpr int8_t lsMaxValue<int8_t>()
{
  return INT8_MAX;
}

template<>
constexpr int16_t lsMaxValue<int16_t>()
{
  return INT16_MAX;
}

template<>
constexpr int32_t lsMaxValue<int32_t>()
{
  return INT32_MAX;
}

template<>
constexpr int64_t lsMaxValue<int64_t>()
{
  return INT64_MAX;
}

template<>
constexpr uint8_t lsMaxValue<uint8_t>()
{
  return UINT8_MAX;
}

template<>
constexpr uint16_t lsMaxValue<uint16_t>()
{
  return UINT16_MAX;
}

template<>
constexpr uint32_t lsMaxValue<uint32_t>()
{
  return UINT32_MAX;
}

template<>
constexpr uint64_t lsMaxValue<uint64_t>()
{
  return UINT64_MAX;
}

template<>
constexpr float_t lsMaxValue<float_t>()
{
  return FLT_MAX;
}

template<>
constexpr double_t lsMaxValue<double_t>()
{
  return DBL_MAX;
}

template<>
constexpr float_t lsSmallest<float_t>()
{
  return FLT_EPSILON;
};

template<>
constexpr double_t lsSmallest<double_t>()
{
  return DBL_EPSILON;
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr inline T lsClampWrap(T val, const T min, const T max)
{
  const T dist = max - min;

  if (max <= min)
    return min;

  if (val < min)
    val += ((min - val + (dist - T(1))) / (dist)) * (dist); // Clamp above min

  return lsMod((val - min), (dist)) + min;
}

//////////////////////////////////////////////////////////////////////////

int64_t lsGetCurrentTimeMs();
int64_t lsGetCurrentTimeNs();
#ifdef LS_ARCH_X64
inline int64_t lsGetCurrentTicks() { return __rdtsc(); }
#elif defined(LS_ARCH_ARM64)
inline int64_t lsGetCurrentTicks()
{
  uint64_t ret;
  asm volatile("mrs %0, cntvct_el0" : "=r" (ret));
  return (int64_t)ret;
}
#endif
uint64_t lsGetRand();

struct rand_seed
{
  uint64_t v[2];

  inline rand_seed() { v[0] = lsGetRand(); v[1] = lsGetRand(); };
  inline rand_seed(const rand_seed &) = default;
  rand_seed &operator =(const rand_seed &) = default;
};

uint64_t lsGetRand(rand_seed &seed);

//////////////////////////////////////////////////////////////////////////

#include "sformat.h"

//////////////////////////////////////////////////////////////////////////

inline void lsPrintToFunction(lsPrintCallbackFunc *pFunc, const char *text)
{
  if (pFunc != nullptr)
    (*pFunc)(text);
}

template <typename ...Args>
inline void lsPrintToFunction(lsPrintCallbackFunc *pFunc, Args ... args)
{
  if (pFunc != nullptr)
    (*pFunc)(sformat(args...));
}

#ifdef LS_PLATFORM_WINDOWS
void lsPrintToOutputWithLength(const char *text, const size_t length);
#endif
void lsPrintToOutput(const char *text);

template <size_t Tcount>
inline void lsPrintToOutputArray(const char(&text)[Tcount])
{
#ifdef LS_PLATFORM_WINDOWS
  lsPrintToOutputWithLength(text, strnlen(text, Tcount));
#else
  lsPrintToOutput(text);
#endif
}

#ifdef LS_PLATFORM_WINDOWS
lsResult lsSetOutputFilePath(const char *path, const bool append = true);
void lsResetOutputFile();
#endif
void lsFlushOutput();

#define print(...) lsPrintToFunction(lsPrintCallback, __VA_ARGS__)
#define print_error_line(...) lsPrintToFunction(lsPrintErrorCallback, __VA_ARGS__)
#define print_log_line(...) lsPrintToFunction(lsPrintLogCallback, __VA_ARGS__)

void print_to_dbgcon(const char *text);

#ifdef LS_PLATFORM_WINDOWS
lsResult lsToWide(_In_ const char *string, const size_t inChars, _Out_ wchar_t *out, const size_t capacity, _Out_ size_t &writtenChars);

inline lsResult lsToWide(_In_ const char *string, _Out_ wchar_t *out, const size_t capacity, _Out_ size_t &writtenChars)
{
  return lsToWide(string, strlen(string) + 1, out, capacity, writtenChars);
}

inline lsResult lsToWide(_In_ const char *string, _Out_ wchar_t *out, const size_t capacity)
{
  size_t _unused;
  return lsToWide(string, out, capacity, _unused);
}
#endif

//////////////////////////////////////////////////////////////////////////

inline bool lsCopyString(char *dst, const size_t dstSize, const char *src, const size_t srcSize)
{
  const size_t max = lsMin(dstSize, srcSize);

  for (size_t i = 0; i < max; i++)
  {
    dst[i] = src[i];

    if (src[i] == '\0')
      return true;
  }

  if (max)
    dst[max - 1] = '\0';

  return false;
}

inline bool lsCopyString(wchar_t *dst, const size_t dstCount, const wchar_t *src, const size_t srcCount)
{
  const size_t max = lsMin(dstCount, srcCount);

  for (size_t i = 0; i < max; i++)
  {
    dst[i] = src[i];

    if (src[i] == '\0')
      return true;
  }

  if (max)
    dst[max - 1] = '\0';

  return false;
}

template <size_t DstSize, size_t SrcSize>
bool lsCopyString(char(&dst)[DstSize], const char(&src)[SrcSize])
{
  return lsCopyString(dst, DstSize, src, SrcSize);
}

template <size_t DstSize>
bool lsCopyString(char(&dst)[DstSize], const char *src, const size_t srcSize)
{
  return lsCopyString(dst, DstSize, src, srcSize);
}

template <size_t DstSize>
bool lsCopyString(char(&dst)[DstSize], const char *src)
{
  return lsCopyString(dst, DstSize, src, (size_t)-1);
}

template <size_t DstSize, size_t SrcSize>
bool lsCopyString(wchar_t(&dst)[DstSize], const wchar_t(&src)[SrcSize])
{
  return lsCopyString(dst, DstSize, src, SrcSize);
}

template <size_t DstSize>
bool lsCopyString(wchar_t(&dst)[DstSize], const wchar_t *src, const size_t srcSize)
{
  return lsCopyString(dst, DstSize, src, srcSize);
}

template <size_t DstSize>
bool lsCopyString(wchar_t(&dst)[DstSize], const wchar_t *src)
{
  return lsCopyString(dst, DstSize, src, (size_t)-1);
}

inline bool lsStringEquals(const char *a, const size_t aSize, const char *b, const size_t bSize)
{
  return strncmp(a, b, lsMin(aSize, bSize)) == 0;
}

template <size_t ASize, size_t BSize>
bool lsStringEquals(const char(&a)[ASize], const char(&b)[BSize])
{
  return lsStringEquals(a, ASize, b, BSize);
}

template <size_t ASize, typename T>
std::enable_if_t<std::is_same<char *, T>::value || std::is_same<const char *, T>::value, bool> lsStringEquals(const char(&a)[ASize], T b)
{
  return lsStringEquals(a, ASize, b, ASize);
}

template <size_t ASize>
bool lsStringEquals(const char(&a)[ASize], const char *b, const size_t bSize)
{
  return lsStringEquals(a, ASize, b, bSize);
}

inline size_t lsStringLength(const char *text, const size_t maxCount)
{
  if (text == nullptr)
    return 0;

  return strnlen(text, maxCount);
}

template <size_t Size>
size_t lsStringLength(const char(&text)[Size])
{
  return lsStringLength(text, Size);
}

template <typename T>
std::enable_if_t<std::is_same<char *, T>::value || std::is_same<const char *, T>::value, size_t> lsStringLength(T text)
{
  return strlen(text);
}

//////////////////////////////////////////////////////////////////////////

int64_t lsParseInt(_In_ const char *start, _Out_ const char **pEnd = nullptr);
uint64_t lsParseUInt(_In_ const char *start, _Out_ const char **pEnd = nullptr);
double_t lsParseFloat(_In_ const char *start, _Out_ const char **pEnd = nullptr);

bool lsIsInt(_In_ const char *text);
bool lsIsInt(_In_ const char *text, const size_t length);
bool lsIsUInt(_In_ const char *text);
bool lsIsUInt(_In_ const char *text, const size_t length);
bool lsIsFloat(_In_ const char *text);
bool lsIsFloat(_In_ const char *text, const size_t length);

bool lsStartsWithInt(_In_ const char *text);
bool lsStartsWithInt(_In_ const char *text, const size_t length);
bool lsStartsWithUInt(_In_ const char *text);
bool lsStartsWithUInt(_In_ const char *text, const size_t length);

int64_t lsParseInt(_In_ const wchar_t *start, _Out_ const wchar_t **pEnd = nullptr);
uint64_t lsParseUInt(_In_ const wchar_t *start, _Out_ const wchar_t **pEnd = nullptr);
double_t lsParseFloat(_In_ const wchar_t *start, _Out_ const wchar_t **pEnd = nullptr);

bool lsIsInt(_In_ const wchar_t *text);
bool lsIsInt(_In_ const wchar_t *text, const size_t length);
bool lsIsUInt(_In_ const wchar_t *text);
bool lsIsUInt(_In_ const wchar_t *text, const size_t length);
bool lsIsFloat(_In_ const wchar_t *text);
bool lsIsFloat(_In_ const wchar_t *text, const size_t length);

bool lsStartsWithInt(_In_ const wchar_t *text);
bool lsStartsWithInt(_In_ const wchar_t *text, const size_t length);
bool lsStartsWithUInt(_In_ const wchar_t *text);
bool lsStartsWithUInt(_In_ const wchar_t *text, const size_t length);

//////////////////////////////////////////////////////////////////////////

template <typename T, typename U>
inline vec2t<U> lsBarycentricInterpolationFactors(const T &p, const T &q, const U &x)
{
  const U wp = (U)(x - q) / (U)(q - p);

  return vec2t<U>(wp, 1 - wp);
}

template <typename T, typename U>
inline vec3t<U> lsBarycentricInterpolationFactors(const vec2t<T> &p, const vec2t<T> &q, const vec2t<T> &r, const vec2t<U> &x)
{
  const U divisor = (U)(p.x * (r.y - q.y) + q.x * (p.y - r.y) + r.x * (q.y - p.y));
  const U wp = (U)(x.x * (r.y - q.y) + q.x * (x.y - r.y) + r.x * (q.y - x.y)) / divisor;
  const U wq = -(U)(x.x * (r.y - p.y) + p.x * (x.y - r.y) + r.x * (p.y - x.y)) / divisor;

  return vec3t<U>(wp, wq, 1 - wp - wq);
}

template <typename T, typename U>
inline vec4t<U> lsBarycentricInterpolationFactors(const vec3t<T> &p, const vec3t<T> &q, const vec3t<T> &r, const vec3t<T> &s, const vec3t<U> &x)
{
  const U val0 = (U)(p.y * (s.z - r.z) + r.y * (p.z - s.z) + (r.z - p.z) * s.y);
  const U val1 = (U)(s.y * (x.z - r.z) + r.y * (s.z - x.z) + (r.z - s.z) * x.y);
  const U val2 = (U)(p.y * (x.z - s.z) + s.y * (p.z - x.z) + (s.z - p.z) * x.y);

  const U divisor = (U)(q.x * val0 + p.x * (r.y * (s.z - q.z) + q.y * (r.z - s.z) + (q.z - r.z) * s.y) + r.x * (q.y * (s.z - p.z) + p.y * (q.z - s.z) + (p.z - q.z) * s.y) + (p.y * (r.z - q.z) + q.y * (p.z - r.z) + (q.z - p.z) * r.y) * s.x);
  const U wp = -(U)(r.x * (q.y * (x.z - s.z) + s.y * (q.z - x.z) + (s.z - q.z) * x.y) + q.x * val1 + s.x * (r.y * (x.z - q.z) + q.y * (r.z - x.z) + (q.z - r.z) * x.y) + (q.y * (s.z - r.z) + r.y * (q.z - s.z) + (r.z - q.z) * s.y) * x.x) / divisor;
  const U wq = (U)(r.x * val2 + p.x * val1 + s.x * (r.y * (x.z - p.z) + p.y * (r.z - x.z) + (p.z - r.z) * x.y) + val0 * x.x) / divisor;
  const U wr = -(U)(q.x * val2 + p.x * (s.y * (x.z - q.z) + q.y * (s.z - x.z) + (q.z - s.z) * x.y) + s.x * (q.y * (x.z - p.z) + p.y * (q.z - x.z) + (p.z - q.z) * x.y) + (p.y * (s.z - q.z) + q.y * (p.z - s.z) + (q.z - p.z) * s.y) * x.x) / divisor;

  return vec4t<U>(wp, wq, wr, 1 - wp - wq - wr);
}

//////////////////////////////////////////////////////////////////////////

template <typename T, typename U>
inline vec2t<U> lsVectorDecomposition(const vec2t<T> &p, const vec2t<T> &q, const vec2t<T> &x)
{
  const U divisor = (U)(p.y * q.x - p.x * q.y);
  const U a = -(U)(q.y * x.x - q.x * x.y) / divisor;
  const U b = (U)(p.y * x.x - p.x * x.y) / divisor;

  return vec2t<U>(a, b);
}

template <typename T, typename U>
inline vec3t<U> lsVectorDecomposition(const vec3t<T> &p, const vec3t<T> &q, const vec3t<T> &r, const vec3t<T> &x)
{
  const U fac0 = (r.z * x.y - r.y * x.z);
  const U fac1 = (q.z * r.y - q.y * r.z);
  const U fac2 = (p.z * q.y - p.y * q.z);
  const U fac3 = (p.y * x.z - p.z * x.y);

  const U divisor = (p.x * fac1 + q.x * (p.y * r.z - p.z * r.y) + fac2 * r.x);

  const U a = (q.x * fac0 + r.x * (q.y * x.z - q.z * x.y) + fac1 * x.x) / divisor;
  const U b = -(p.x * fac0 + r.x * fac3 + (p.z * r.y - p.y * r.z) * x.x) / divisor;
  const U c = (p.x * (q.z * x.y - q.y * x.z) + q.x * fac3 + fac2 * x.x) / divisor;

  return vec3t<U>(a, b, c);
}

//////////////////////////////////////////////////////////////////////////

inline constexpr vec3f lsColor_UnpackBgraToVec3f(const uint32_t bgraColor)
{
  constexpr float_t v(1.0f / 0xFF);
  return vec3f((float_t)((bgraColor & 0x00FF0000) >> 0x10), (float_t)((bgraColor & 0x0000FF00) >> 0x8), (float_t)(bgraColor & 0x000000FF)) * v;
}

inline constexpr vec3d lsColor_UnpackBgraToVec3d(const uint32_t bgraColor)
{
  constexpr double_t v(1.0 / 0xFF);
  return vec3d((double_t)((bgraColor & 0x00FF0000) >> 0x10), (double_t)((bgraColor & 0x0000FF00) >> 0x8), (double_t)(bgraColor & 0x000000FF)) * v;
}

inline constexpr vec4f lsColor_UnpackBgraToVec4f(const uint32_t bgraColor)
{
  constexpr const float_t v(1.0f / 0xFF);
  return vec4f((float_t)((bgraColor & 0x00FF0000) >> 0x10), (float_t)((bgraColor & 0x0000FF00) >> 0x8), (float_t)(bgraColor & 0x000000FF), (float_t)((bgraColor & 0xFF000000) >> 0x18)) * v;
}

inline constexpr vec4d lsColor_UnpackBgraToVec4d(const uint32_t bgraColor)
{
  constexpr double_t v(1.0 / 0xFF);
  return vec4d((double_t)((bgraColor & 0x00FF0000) >> 0x10), (double_t)((bgraColor & 0x0000FF00) >> 0x8), (double_t)(bgraColor & 0x000000FF), (double_t)((bgraColor & 0xFF000000) >> 0x18)) * v;
}

inline constexpr uint32_t lsColor_PackVec3fToBgra(const vec3f rgbVector)
{
  constexpr float_t v = (float_t)0xFF;
  const vec3f v0 = rgbVector * v;
  return (lsClamp((uint32_t)v0.x, 0u, 0xFFu) << 0x10) | (lsClamp((uint32_t)v0.y, 0u, 0xFFu) << 0x8) | lsClamp((uint32_t)v0.z, 0u, 0xFFu) | 0xFF000000;
}

inline constexpr uint32_t lsColor_PackVec3dToBgra(const vec3d rgbVector)
{
  constexpr double_t v = (double_t)0xFF;
  const vec3d v0 = rgbVector * v;
  return (lsClamp((uint32_t)v0.x, 0u, 0xFFu) << 0x10) | (lsClamp((uint32_t)v0.y, 0u, 0xFFu) << 0x8) | lsClamp((uint32_t)v0.z, 0u, 0xFFu) | 0xFF000000;
}

inline constexpr uint32_t lsColor_PackVec4fToBgra(const vec4f rgbaVector)
{
  constexpr float_t v = (float_t)0xFF;
  const vec4f v0 = rgbaVector * v;
  return (lsClamp((uint32_t)v0.x, 0u, 0xFFu) << 0x10) | (lsClamp((uint32_t)v0.y, 0u, 0xFFu) << 0x8) | lsClamp((uint32_t)v0.z, 0u, 0xFFu) | (lsClamp((uint32_t)v0.w, 0u, 0xFFu) << 0x18);
}

inline constexpr uint32_t lsColor_PackVec4dToBgra(const vec4d rgbaVector)
{
  constexpr double_t v = (double_t)0xFF;
  const vec4d v0 = rgbaVector * v;
  return (lsClamp((uint32_t)v0.x, 0u, 0xFFu) << 0x10) | (lsClamp((uint32_t)v0.y, 0u, 0xFFu) << 0x8) | lsClamp((uint32_t)v0.z, 0u, 0xFFu) | (lsClamp((uint32_t)v0.w, 0u, 0xFFu) << 0x18);
}

inline constexpr vec3f lsColor_UnpackRgbaToVec3f(const uint32_t rgbaColor)
{
  constexpr float_t v(1.0f / 0xFF);
  return vec3f((float_t)(rgbaColor & 0x000000FF), (float_t)((rgbaColor & 0x0000FF00) >> 0x8), (float_t)((rgbaColor & 0x00FF0000) >> 0x10)) * v;
}

inline constexpr vec3d lsColor_UnpackRgbaToVec3d(const uint32_t rgbaColor)
{
  constexpr double_t v(1.0 / 0xFF);
  return vec3d((double_t)(rgbaColor & 0x000000FF), (double_t)((rgbaColor & 0x0000FF00) >> 0x8), (double_t)((rgbaColor & 0x00FF0000) >> 0x10)) * v;
}

inline constexpr vec4f lsColor_UnpackRgbaToVec4f(const uint32_t rgbaColor)
{
  constexpr const float_t v(1.0f / 0xFF);
  return vec4f((float_t)(rgbaColor & 0x000000FF), (float_t)((rgbaColor & 0x0000FF00) >> 0x8), (float_t)((rgbaColor & 0x00FF0000) >> 0x10), (float_t)((rgbaColor & 0xFF000000) >> 0x18)) * v;
}

inline constexpr vec4d lsColor_UnpackRgbaToVec4d(const uint32_t rgbaColor)
{
  constexpr double_t v(1.0 / 0xFF);
  return vec4d((double_t)(rgbaColor & 0x000000FF), (double_t)((rgbaColor & 0x0000FF00) >> 0x8), (double_t)((rgbaColor & 0x00FF0000) >> 0x10), (double_t)((rgbaColor & 0xFF000000) >> 0x18)) * v;
}

inline constexpr uint32_t lsColor_PackVec3fToRgba(const vec3f rgbVector)
{
  constexpr float_t v = (float_t)0xFF;
  const vec3f v0 = rgbVector * v;
  return (lsClamp((uint32_t)v0.z, 0u, 0xFFu) << 0x10) | (lsClamp((uint32_t)v0.y, 0u, 0xFFu) << 0x8) | lsClamp((uint32_t)v0.x, 0u, 0xFFu) | 0xFF000000;
}

inline constexpr uint32_t lsColor_PackVec3dToRgba(const vec3d rgbVector)
{
  constexpr double_t v = (double_t)0xFF;
  const vec3d v0 = rgbVector * v;
  return (lsClamp((uint32_t)v0.z, 0u, 0xFFu) << 0x10) | (lsClamp((uint32_t)v0.y, 0u, 0xFFu) << 0x8) | lsClamp((uint32_t)v0.x, 0u, 0xFFu) | 0xFF000000;
}

inline constexpr uint32_t lsColor_PackVec4fToRgba(const vec4f rgbaVector)
{
  constexpr float_t v = (float_t)0xFF;
  const vec4f v0 = rgbaVector * v;
  return (lsClamp((uint32_t)v0.z, 0u, 0xFFu) << 0x10) | (lsClamp((uint32_t)v0.y, 0u, 0xFFu) << 0x8) | lsClamp((uint32_t)v0.x, 0u, 0xFFu) | (lsClamp((uint32_t)v0.w, 0u, 0xFFu) << 0x18);
}

inline constexpr uint32_t lsColor_PackVec4dToRgba(const vec4d rgbaVector)
{
  constexpr double_t v = (double_t)0xFF;
  const vec4d v0 = rgbaVector * v;
  return (lsClamp((uint32_t)v0.z, 0u, 0xFFu) << 0x10) | (lsClamp((uint32_t)v0.y, 0u, 0xFFu) << 0x8) | lsClamp((uint32_t)v0.x, 0u, 0xFFu) | (lsClamp((uint32_t)v0.w, 0u, 0xFFu) << 0x18);
}

inline vec3f lsColor_HueToVec3f(const float_t hue)
{
  const float_t h = hue * 6;
  const float_t r = lsAbs(h - 3) - 1;
  const float_t g = 2 - lsAbs(h - 2);
  const float_t b = 2 - lsAbs(h - 4);

  return vec3f(lsClamp(r, 0.0f, 1.0f), lsClamp(g, 0.0f, 1.0f), lsClamp(b, 0.0f, 1.0f));
}

inline uint32_t lsColor_HueToBgra(const float_t hue)
{
  return lsColor_PackVec3fToBgra(lsColor_HueToVec3f(hue));
}

inline vec3f lsColor_YuvToRgb(const vec3f yuv)
{
  return vec3f(lsClamp(yuv.x + 1.370705f * (yuv.z - .5f), 0.f, 1.f),
    lsClamp(yuv.x - .698001f * (yuv.z - .5f) - 0.337633f * (yuv.y - .5f), 0.f, 1.f),
    lsClamp(yuv.x + 1.732446f * (yuv.y - .5f), 0.f, 1.f));
}

inline vec3f lsColor_RgbToYuv(const vec3f rgb)
{
  return vec3f(lsClamp(.2988222643743755f * rgb.x + .5868145917975452f * rgb.y + .1143631438280793f * rgb.z, 0.f, 1.f),
    lsClamp(-(.1724857596567948f * rgb.x + .3387202786104417f * rgb.y - .5112060382672364f * rgb.z - .5f), 0.f, 1.f),
    lsClamp(.5115453256722814f * rgb.x - .4281115132705763f * rgb.y - .08343381240170515f * rgb.z + .5f, 0.f, 1.f));
}

inline vec3f lsColor_RgbToHcv(const vec3f rgb)
{
  const vec4f p = (rgb.y < rgb.z) ? vec4f(rgb.z, rgb.y, -1.f, 2.f / 3.f) : vec4f(rgb.y, rgb.z, 0.f, -1.f / 3.f);
  const vec4f q = (rgb.x < p.x) ? vec4f(p.x, p.y, p.w, rgb.x) : vec4f(rgb.x, p.y, p.z, p.x);
  const float_t c = q.x - lsMin(q.w, q.y);
  const float_t h = lsAbs((q.w - q.y) / (6.f * c + FLT_EPSILON) + q.z);

  return vec3f(h, c, q.x);
}

inline vec3f lsColor_HslToRgb(const vec3f hsl)
{
  const vec3f rgb = lsColor_HueToVec3f(hsl.x);
  const float_t c = (1.f - lsAbs(2.f * hsl.z - 1.f)) * hsl.y;

  return (rgb - vec3f(.5f)) * c + vec3f(hsl.z);
}

inline vec3f lsColor_RgbToHsl(const vec3f rgb)
{
  const vec3f hcv = lsColor_RgbToHcv(rgb);
  const float_t l = hcv.z - hcv.y * .5f;
  const float_t s = hcv.y / (1.f - lsAbs(l * 2.f - 1.f) + FLT_EPSILON);

  return vec3f(hcv.x, s, l);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
inline size_t sformat_GetMaxBytes(const vec2t<T> &, const sformatState &fs)
{
  constexpr size_t dimensions = 2;

  return 1 + (size_t)fs.vectorSpaceAfterStart + dimensions * sformat_GetMaxBytes((T)0, fs) + (dimensions - 1) * ((size_t)fs.vectorSpaceAfterSeparator + 1) + (size_t)fs.vectorSpaceBeforeEnd + 1;
}

template <typename T>
inline size_t _sformat_Append(const vec2t<T> &value, const sformatState &fs, char *text)
{
  return _sformat_AppendVector(&value.x, 2, fs, text);
}

template <typename T>
inline size_t sformat_GetMaxBytes(const vec3t<T> &, const sformatState &fs)
{
  constexpr size_t dimensions = 3;

  return 1 + (size_t)fs.vectorSpaceAfterStart + dimensions * sformat_GetMaxBytes((T)0, fs) + (dimensions - 1) * ((size_t)fs.vectorSpaceAfterSeparator + 1) + (size_t)fs.vectorSpaceBeforeEnd + 1;
}

template <typename T>
inline size_t _sformat_Append(const vec3t<T> &value, const sformatState &fs, char *text)
{
  return _sformat_AppendVector(&value.x, 3, fs, text);
}

template <typename T>
inline size_t sformat_GetMaxBytes(const vec4t<T> &, const sformatState &fs)
{
  constexpr size_t dimensions = 4;

  return 1 + (size_t)fs.vectorSpaceAfterStart + dimensions * sformat_GetMaxBytes((T)0, fs) + (dimensions - 1) * ((size_t)fs.vectorSpaceAfterSeparator + 1) + (size_t)fs.vectorSpaceBeforeEnd + 1;
}

template <typename T>
inline size_t _sformat_Append(const vec4t<T> &value, const sformatState &fs, char *text)
{
  return _sformat_AppendVector(&value.x, 4, fs, text);
}

//////////////////////////////////////////////////////////////////////////

template <typename TObj, typename T>
concept random_iterable_sized_container_type = requires(const TObj & t, size_t idx) {
  { t.count } -> std::convertible_to<size_t>;
  { t[idx] } -> std::convertible_to<T>;
};

template <typename T>
struct const_ptr_wrapper
{
  const T *pData;
  const size_t count;

  inline const_ptr_wrapper(const T *pData, const size_t count) : pData(pData), count(count) { { lsAssert(pData != nullptr || count == 0); } }

  inline const T &operator [] (const size_t index) const
  {
    { lsAssert(index < count); }
    return pData[index];
  };
};

template <typename T, size_t TCount>
struct const_array_wrapper
{
  const T *pData;
  constexpr static size_t count = TCount;

  inline const_array_wrapper(const T(&arr)[TCount]) : pData(arr) {}

  inline const T &operator [] (const size_t index) const
  {
    { lsAssert(index < TCount); }
    return pData[index];
  };
};

//////////////////////////////////////////////////////////////////////////

template <uint8_t bits>
struct bits_to_uint
{
  static_assert(bits != 0 && bits <= 63);
  using type = uint64_t;
};

template <> struct bits_to_uint<0> { using type = uint8_t; };
template <> struct bits_to_uint<1> { using type = uint8_t; };
template <> struct bits_to_uint<2> { using type = uint8_t; };
template <> struct bits_to_uint<3> { using type = uint8_t; };
template <> struct bits_to_uint<4> { using type = uint8_t; };
template <> struct bits_to_uint<5> { using type = uint8_t; };
template <> struct bits_to_uint<6> { using type = uint8_t; };
template <> struct bits_to_uint<7> { using type = uint8_t; };
template <> struct bits_to_uint<8> { using type = uint16_t; };
template <> struct bits_to_uint<9> { using type = uint16_t; };
template <> struct bits_to_uint<10> { using type = uint16_t; };
template <> struct bits_to_uint<11> { using type = uint16_t; };
template <> struct bits_to_uint<12> { using type = uint16_t; };
template <> struct bits_to_uint<13> { using type = uint16_t; };
template <> struct bits_to_uint<14> { using type = uint16_t; };
template <> struct bits_to_uint<15> { using type = uint16_t; };
template <> struct bits_to_uint<16> { using type = uint32_t; };
template <> struct bits_to_uint<17> { using type = uint32_t; };
template <> struct bits_to_uint<18> { using type = uint32_t; };
template <> struct bits_to_uint<19> { using type = uint32_t; };
template <> struct bits_to_uint<20> { using type = uint32_t; };
template <> struct bits_to_uint<21> { using type = uint32_t; };
template <> struct bits_to_uint<22> { using type = uint32_t; };
template <> struct bits_to_uint<23> { using type = uint32_t; };
template <> struct bits_to_uint<24> { using type = uint32_t; };
template <> struct bits_to_uint<25> { using type = uint32_t; };
template <> struct bits_to_uint<26> { using type = uint32_t; };
template <> struct bits_to_uint<27> { using type = uint32_t; };
template <> struct bits_to_uint<28> { using type = uint32_t; };
template <> struct bits_to_uint<29> { using type = uint32_t; };
template <> struct bits_to_uint<30> { using type = uint32_t; };
template <> struct bits_to_uint<31> { using type = uint32_t; };

template <uint64_t value>
using count_to_uint = std::conditional_t<value == 0, uint8_t, typename bits_to_uint<(uint8_t)lsHighestBit(value - 1)>::type>;

template <uint64_t value>
using value_to_uint = std::conditional_t<value == 0, uint8_t, typename bits_to_uint<(uint8_t)lsHighestBit(value)>::type>;
