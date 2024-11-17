#include "core.h"

#ifdef LS_PLATFORM_WINDOWS
#include <winnt.h>
#include <fcntl.h>
#include <corecrt_io.h>
#endif

//////////////////////////////////////////////////////////////////////////

bool _ls_error_silent_global = false;
thread_local bool _ls_error_silent = _ls_error_silent_global;

bool _ls_error_break_global = true;
thread_local bool _ls_error_break = _ls_error_break_global;

//////////////////////////////////////////////////////////////////////////

const char *lsResult_to_string(const lsResult result)
{
  const char *lut[] =
  {
    "lsR_Success",
    "lsR_Failure",
    "lsR_InvalidParameter",
    "lsR_ArgumentNull",
    "lsR_IOFailure",
    "lsR_ArgumentOutOfBounds",
    "lsR_ResourceInvalid",
    "lsR_ResourceIncompatible",
    "lsR_ResourceNotFound",
    "lsR_ResourceStateInvalid",
    "lsR_ResourceAlreadyExists",
    "lsR_ResourceBusy",
    "lsR_MemoryAllocationFailure",
    "lsR_OperationNotSupported",
    "lsR_NotSupported",
    "lsR_EndOfStream",
    "lsR_InternalError",
    "lsR_ResourceFull",
    "lsR_ResourceInsufficient",
  };

  static_assert(LS_ARRAYSIZE(lut) == _lsResult_Count, "Results missing in `lsResult_to_string`!");

  if ((size_t)result < LS_ARRAYSIZE(lut))
    return lut[result];
  else
    return "INVALID_RESULT";
}

bool _ls_error_handle_direct(const lsResult result, const char *expression, const char *file, const uint32_t line)
{
  if (!_ls_error_silent)
  {
    char buffer[2048];
    sformat_to(buffer, LS_ARRAYSIZE(buffer), "Error '", lsResult_to_string(result), "' in File '", file, "' (Line ", line, ")");
    lsPrintErrorCallback(buffer);

    if (expression)
    {
      sformat_to(buffer, LS_ARRAYSIZE(buffer), "Expression: ", expression);
      lsPrintErrorCallback(buffer);
    }
  }

  return _ls_error_break;
}

bool _ls_error_handle_indirect(const lsResult result, const char *expression, const char *file, const uint32_t line)
{
  return _ls_error_handle_direct(result, expression, file, line);
}

//////////////////////////////////////////////////////////////////////////

int64_t lsGetCurrentTimeMs()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int64_t lsGetCurrentTimeNs()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

uint64_t lsGetRand()
{
#ifdef LS_PLATFORM_WINDOWS
  LS_ALIGN(16) static uint64_t last[2] = { (uint64_t)lsGetCurrentTimeNs(), __rdtsc() };
  LS_ALIGN(16) static uint64_t last2[2] = { ~__rdtsc(), ~(uint64_t)lsGetCurrentTimeNs() };

  const __m128i a = _mm_load_si128(reinterpret_cast<__m128i *>(last));
  const __m128i b = _mm_load_si128(reinterpret_cast<__m128i *>(last2));

  const __m128i r = _mm_aesdec_si128(a, b);

  _mm_store_si128(reinterpret_cast<__m128i *>(last), b);
  _mm_store_si128(reinterpret_cast<__m128i *>(last2), r);

  return last[1] ^ last[0];
#else
  static uint64_t last[2] = { (uint64_t)lsGetCurrentTimeNs(), (uint64_t)lsGetCurrentTicks() };
  
  const uint64_t oldstate_hi = last[0];
  const uint64_t oldstate_lo = oldstate_hi * 6364136223846793005ULL + (last[1] | 1);
  last[0] = oldstate_hi * 6364136223846793005ULL + (last[1] | 1);
  
  const uint32_t xorshifted_hi = (uint32_t)(((oldstate_hi >> 18) ^ oldstate_hi) >> 27);
  const uint32_t rot_hi = (uint32_t)(oldstate_hi >> 59);
  
  const uint32_t xorshifted_lo = (uint32_t)(((oldstate_lo >> 18) ^ oldstate_lo) >> 27);
  const uint32_t rot_lo = (uint32_t)(oldstate_lo >> 59);
  
  const uint32_t hi = (xorshifted_hi >> rot_hi) | (xorshifted_hi << (uint32_t)((-(int32_t)rot_hi) & 31));
  const uint32_t lo = (xorshifted_lo >> rot_lo) | (xorshifted_lo << (uint32_t)((-(int32_t)rot_lo) & 31));
  
  return ((uint64_t)hi << 32) | lo;
#endif
}

uint64_t lsGetRand(rand_seed &seed)
{
  const uint64_t oldstate_hi = seed.v[0];
  const uint64_t oldstate_lo = oldstate_hi * 6364136223846793005ULL + (seed.v[1] | 1);
  seed.v[0] = oldstate_hi * 6364136223846793005ULL + (seed.v[1] | 1);
  
  const uint32_t xorshifted_hi = (uint32_t)(((oldstate_hi >> 18) ^ oldstate_hi) >> 27);
  const uint32_t rot_hi = (uint32_t)(oldstate_hi >> 59);
  
  const uint32_t xorshifted_lo = (uint32_t)(((oldstate_lo >> 18) ^ oldstate_lo) >> 27);
  const uint32_t rot_lo = (uint32_t)(oldstate_lo >> 59);
  
  const uint32_t hi = (xorshifted_hi >> rot_hi) | (xorshifted_hi << (uint32_t)((-(int32_t)rot_hi) & 31));
  const uint32_t lo = (xorshifted_lo >> rot_lo) | (xorshifted_lo << (uint32_t)((-(int32_t)rot_lo) & 31));
  
  return ((uint64_t)hi << 32) | lo;
}

//////////////////////////////////////////////////////////////////////////

int64_t lsParseInt(_In_ const char *start, _Out_ const char **pEnd /* = nullptr */)
{
  const char *endIfNoEnd = nullptr;

  if (pEnd == nullptr)
    pEnd = &endIfNoEnd;

  int64_t ret = 0;

  // See: https://graphics.stanford.edu/~seander/bithacks.html#ConditionalNegate
  int64_t negate = 0;

  if (*start == '-')
  {
    negate = 1;
    start++;
  }

  while (true)
  {
    uint8_t digit = *start - '0';

    if (digit > 9)
      break;

    ret = (ret << 1) + (ret << 3) + digit;
    start++;
  }

  *pEnd = start;

  return (ret ^ -negate) + negate;
}

uint64_t lsParseUInt(_In_ const char *start, _Out_ const char **pEnd /* = nullptr */)
{
  const char *endIfNoEnd = nullptr;

  if (pEnd == nullptr)
    pEnd = &endIfNoEnd;

  uint64_t ret = 0;

  while (true)
  {
    uint8_t digit = *start - '0';

    if (digit > 9)
      break;

    ret = (ret << 1) + (ret << 3) + digit;
    start++;
  }

  *pEnd = start;

  return ret;
}

double_t lsParseFloat(_In_ const char *start, _Out_ const char **pEnd /* = nullptr */)
{
  const char *endIfNoEnd = nullptr;

  if (pEnd == nullptr)
    pEnd = &endIfNoEnd;

  uint64_t sign = 0;

  if (*start == '-')
  {
    sign = (uint64_t)1 << 63; // IEEE floating point signed bit.
    ++start;
  }

  const char *_end = start;
  const int64_t left = lsParseUInt(start, &_end);
  double_t ret = (double_t)left;

  if (*_end == '.')
  {
    start = _end + 1;
    const int64_t right = lsParseUInt(start, &_end);

    const double_t fracMult[] = { 0.0, 1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6, 1e-7, 1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13 };

    if (_end - start < (ptrdiff_t)LS_ARRAYSIZE(fracMult))
      ret = (ret + right * fracMult[_end - start]);
    else
      ret = (ret + right * pow(10, _end - start));

    // Get Sign. (memcpy should get optimized away and is only there to prevent undefined behavior)
    {
      uint64_t data;
      static_assert(sizeof(data) == sizeof(ret), "Platform not supported.");
      memcpy(&data, &ret, sizeof(data));
      data ^= sign;
      memcpy(&ret, &data, sizeof(data));
    }

    *pEnd = _end;

    if (*_end == 'e' || *_end == 'E')
    {
      start = ++_end;

      if ((*start >= '0' && *start <= '9') || *start == '-')
      {
        ret *= pow(10, lsParseInt(start, &_end));

        *pEnd = _end;
      }
    }
  }
  else
  {
    // Get Sign. (memcpy should get optimized away and is only there to prevent undefined behavior)
    {
      uint64_t data;
      static_assert(sizeof(data) == sizeof(ret), "Platform not supported.");
      memcpy(&data, &ret, sizeof(data));
      data ^= sign;
      memcpy(&ret, &data, sizeof(data));
    }

    if (*_end == 'e' || *_end == 'E')
    {
      start = ++_end;

      if ((*start >= '0' && *start <= '9') || *start == '-')
        ret *= pow(10, lsParseInt(start, &_end));
    }

    *pEnd = _end;
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////

bool lsIsInt(_In_ const char *text)
{
  if (text == nullptr)
    return false;

  return lsIsInt(text, strlen(text));
}

bool lsIsInt(_In_ const char *text, const size_t length)
{
  if (text == nullptr)
    return false;

  const bool sign = (text[0] == '-');
  size_t i = (size_t)sign;

  for (; i < length; i++)
  {
    if (text[i] < '0' || text[i] > '9')
    {
      if (text[i] == '\0')
        return (i > (size_t)sign);

      return false;
    }
  }

  return i > (size_t)sign;
}

//////////////////////////////////////////////////////////////////////////

bool lsIsUInt(_In_ const char *text)
{
  if (text == nullptr)
    return false;

  return lsIsUInt(text, strlen(text));
}

bool lsIsUInt(_In_ const char *text, const size_t length)
{
  if (text == nullptr)
    return false;

  size_t i = 0;

  for (; i < length; i++)
  {
    if (text[i] < '0' || text[i] > '9')
    {
      if (text[i] == '\0')
        return i > 0;

      return false;
    }
  }

  return i > 0;
}

//////////////////////////////////////////////////////////////////////////

bool lsIsFloat(_In_ const char *text)
{
  if (text == nullptr)
    return false;

  return lsIsFloat(text, strlen(text));
}

bool lsIsFloat(_In_ const char *text, const size_t length)
{
  if (text == nullptr)
    return false;

  bool hasDigits = false;
  bool hasPostPeriodDigits = false;
  bool hasPostExponentDigits = false;

  size_t i = (size_t)(text[0] == '-');

  for (; i < length; i++)
  {
    if (text[i] == '\0')
    {
      return hasDigits;
    }
    else if (text[i] == '.')
    {
      i++;
      goto period;
    }
    else if (text[i] == 'e' || text[i] == 'E')
    {
      if (!hasDigits)
        return false;

      i++;
      goto exponent;
    }
    else if (text[i] < '0' || text[i] > '9')
    {
      return false;
    }
    else
    {
      hasDigits = true;
    }
  }

  return hasDigits;

period:
  for (; i < length; i++)
  {
    if (text[i] == '\0')
    {
      return hasDigits || hasPostPeriodDigits;
    }
    else if (text[i] == 'e' || text[i] == 'E')
    {
      if (!(hasDigits || hasPostPeriodDigits))
        return false;

      i++;
      goto exponent;
    }
    else if (text[i] < '0' || text[i] > '9')
    {
      return false;
    }
    else
    {
      hasPostPeriodDigits = true;
    }
  }

  return hasDigits || hasPostPeriodDigits;

exponent:
  i += (size_t)(text[i] == '-');

  for (; i < length; i++)
  {
    if (text[i] < '0' || text[i] > '9')
    {
      if (text[i] == '\0')
        return hasPostExponentDigits && (hasPostPeriodDigits || hasDigits);

      return false;
    }
    else
    {
      hasPostExponentDigits = true;
    }
  }

  return hasPostExponentDigits && (hasPostPeriodDigits || hasDigits);
}

//////////////////////////////////////////////////////////////////////////

bool lsStartsWithInt(_In_ const char *text)
{
  if (text == nullptr)
    return false;

  return lsStartsWithInt(text, strlen(text));
}

bool lsStartsWithInt(_In_ const char *text, const size_t length)
{
  if (text == nullptr)
    return false;

  const bool sign = (text[0] == '-');
  size_t i = (size_t)sign;

  for (; i < length; i++)
    if (text[i] < '0' || text[i] > '9')
      return (i > (size_t)sign);

  return i > (size_t)sign;
}

//////////////////////////////////////////////////////////////////////////

bool lsStartsWithUInt(_In_ const char *text)
{
  if (text == nullptr)
    return false;

  return lsStartsWithUInt(text, strlen(text));
}

bool lsStartsWithUInt(_In_ const char *text, const size_t length)
{
  if (text == nullptr)
    return false;

  size_t i = 0;

  for (; i < length; i++)
    if (text[i] < '0' || text[i] > '9')
      return i > 0;

  return i > 0;
}

//////////////////////////////////////////////////////////////////////////

int64_t lsParseInt(_In_ const wchar_t *start, _Out_ const wchar_t **pEnd /* = nullptr */)
{
  const wchar_t *endIfNoEnd = nullptr;

  if (pEnd == nullptr)
    pEnd = &endIfNoEnd;

  int64_t ret = 0;

  // See: https://graphics.stanford.edu/~seander/bithacks.html#ConditionalNegate
  int64_t negate = 0;

  if (*start == L'-')
  {
    negate = 1;
    start++;
  }

  while (true)
  {
    uint16_t digit = *start - L'0';

    if (digit > 9)
      break;

    ret = (ret << 1) + (ret << 3) + digit;
    start++;
  }

  *pEnd = start;

  return (ret ^ -negate) + negate;
}

uint64_t lsParseUInt(_In_ const wchar_t *start, _Out_ const wchar_t **pEnd /* = nullptr */)
{
  const wchar_t *endIfNoEnd = nullptr;

  if (pEnd == nullptr)
    pEnd = &endIfNoEnd;

  uint64_t ret = 0;

  while (true)
  {
    uint16_t digit = *start - L'0';

    if (digit > 9)
      break;

    ret = (ret << 1) + (ret << 3) + digit;
    start++;
  }

  *pEnd = start;

  return ret;
}

double_t lsParseFloat(_In_ const wchar_t *start, _Out_ const wchar_t **pEnd /* = nullptr */)
{
  const wchar_t *endIfNoEnd = nullptr;

  if (pEnd == nullptr)
    pEnd = &endIfNoEnd;

  uint64_t sign = 0;

  if (*start == L'-')
  {
    sign = (uint64_t)1 << 63; // IEEE floating point signed bit.
    ++start;
  }

  const wchar_t *_end = start;
  const int64_t left = lsParseInt(start, &_end);
  double_t ret = (double_t)left;

  if (*_end == L'.')
  {
    start = _end + 1;
    const int64_t right = lsParseInt(start, &_end);

    const double_t fracMult[] = { 0.0, 1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6, 1e-7, 1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13 };

    if (_end - start < (ptrdiff_t)LS_ARRAYSIZE(fracMult))
      ret = (ret + right * fracMult[_end - start]);
    else
      ret = (ret + right * pow(10, _end - start));

    // Get Sign. (memcpy should get optimized away and is only there to prevent undefined behavior)
    {
      uint64_t data;
      static_assert(sizeof(data) == sizeof(ret), "Platform not supported.");
      memcpy(&data, &ret, sizeof(data));
      data ^= sign;
      memcpy(&ret, &data, sizeof(data));
    }

    *pEnd = _end;

    if (*_end == L'e' || *_end == L'E')
    {
      start = ++_end;

      if ((*start >= L'0' && *start <= L'9') || *start == L'-')
      {
        ret *= pow(10, lsParseInt(start, &_end));

        *pEnd = _end;
      }
    }
  }
  else
  {
    // Get Sign. (memcpy should get optimized away and is only there to prevent undefined behavior)
    {
      uint64_t data;
      static_assert(sizeof(data) == sizeof(ret), "Platform not supported.");
      memcpy(&data, &ret, sizeof(data));
      data ^= sign;
      memcpy(&ret, &data, sizeof(data));
    }

    if (*_end == L'e' || *_end == L'E')
    {
      start = ++_end;

      if ((*start >= L'0' && *start <= L'9') || *start == L'-')
        ret *= pow(10, lsParseInt(start, &_end));
    }

    *pEnd = _end;
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////

bool lsIsInt(_In_ const wchar_t *text)
{
  if (text == nullptr)
    return false;

  return lsIsInt(text, wcslen(text));
}

bool lsIsInt(_In_ const wchar_t *text, const size_t length)
{
  if (text == nullptr)
    return false;

  const bool sign = (text[0] == L'-');
  size_t i = (size_t)sign;

  for (; i < length; i++)
  {
    if (text[i] < L'0' || text[i] > L'9')
    {
      if (text[i] == L'\0')
        return (i > (size_t)sign);

      return false;
    }
  }

  return i > (size_t)sign;
}

//////////////////////////////////////////////////////////////////////////

bool lsIsUInt(_In_ const wchar_t *text)
{
  if (text == nullptr)
    return false;

  return lsIsUInt(text, wcslen(text));
}

bool lsIsUInt(_In_ const wchar_t *text, const size_t length)
{
  if (text == nullptr)
    return false;

  size_t i = 0;

  for (; i < length; i++)
  {
    if (text[i] < L'0' || text[i] > L'9')
    {
      if (text[i] == L'\0')
        return i > 0;

      return false;
    }
  }

  return i > 0;
}

//////////////////////////////////////////////////////////////////////////

bool lsIsFloat(_In_ const wchar_t *text)
{
  if (text == nullptr)
    return false;

  return lsIsFloat(text, wcslen(text));
}

bool lsIsFloat(_In_ const wchar_t *text, const size_t length)
{
  if (text == nullptr)
    return false;

  bool hasDigits = false;
  bool hasPostPeriodDigits = false;
  bool hasPostExponentDigits = false;

  size_t i = (size_t)(text[0] == L'-');

  for (; i < length; i++)
  {
    if (text[i] == L'\0')
    {
      return hasDigits;
    }
    else if (text[i] == L'.')
    {
      i++;
      goto period;
    }
    else if (text[i] == L'e' || text[i] == L'E')
    {
      if (!hasDigits)
        return false;

      i++;
      goto exponent;
    }
    else if (text[i] < L'0' || text[i] > L'9')
    {
      return false;
    }
    else
    {
      hasDigits = true;
    }
  }

  return hasDigits;

period:
  for (; i < length; i++)
  {
    if (text[i] == L'\0')
    {
      return hasDigits || hasPostPeriodDigits;
    }
    else if (text[i] == L'e' || text[i] == L'E')
    {
      if (!(hasDigits || hasPostPeriodDigits))
        return false;

      i++;
      goto exponent;
    }
    else if (text[i] < L'0' || text[i] > L'9')
    {
      return false;
    }
    else
    {
      hasPostPeriodDigits = true;
    }
  }

  return hasDigits || hasPostPeriodDigits;

exponent:
  i += (size_t)(text[i] == L'-');

  for (; i < length; i++)
  {
    if (text[i] < L'0' || text[i] > L'9')
    {
      if (text[i] == L'\0')
        return hasPostExponentDigits && (hasPostPeriodDigits || hasDigits);

      return false;
    }
    else
    {
      hasPostExponentDigits = true;
    }
  }

  return hasPostExponentDigits && (hasPostPeriodDigits || hasDigits);
}

//////////////////////////////////////////////////////////////////////////

bool lsStartsWithInt(_In_ const wchar_t *text)
{
  if (text == nullptr)
    return false;

  return lsStartsWithInt(text, wcslen(text));
}

bool lsStartsWithInt(_In_ const wchar_t *text, const size_t length)
{
  if (text == nullptr)
    return false;

  const bool sign = (text[0] == L'-');
  size_t i = (size_t)sign;

  for (; i < length; i++)
    if (text[i] < L'0' || text[i] > L'9')
      return (i > (size_t)sign);

  return i > (size_t)sign;
}

//////////////////////////////////////////////////////////////////////////

bool lsStartsWithUInt(_In_ const wchar_t *text)
{
  if (text == nullptr)
    return false;

  return lsStartsWithUInt(text, wcslen(text));
}

bool lsStartsWithUInt(_In_ const wchar_t *text, const size_t length)
{
  if (text == nullptr)
    return false;

  size_t i = 0;

  for (; i < length; i++)
    if (text[i] < L'0' || text[i] > L'9')
      return i > 0;

  return i > 0;
}

//////////////////////////////////////////////////////////////////////////

#ifdef LS_PLATFORM_WINDOWS
HANDLE lsStdOutHandle = nullptr;
HANDLE lsFileOutHandle = nullptr;
constexpr bool lsStdOutVTCodeColors = false; // Disabled by default. Code Path works but is probably slower and adds more clutter.
bool lsStdOutForceStdIO = false;
FILE *pStdOut = nullptr;
#endif

inline void lsInitConsole()
{
#ifdef LS_PLATFORM_WINDOWS
  if (lsStdOutHandle == nullptr)
  {
    lsStdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (lsStdOutHandle == INVALID_HANDLE_VALUE)
      lsStdOutHandle = nullptr;

    if (lsStdOutHandle != nullptr)
    {
      DWORD consoleMode = 0;
      lsStdOutForceStdIO = !GetConsoleMode(lsStdOutHandle, &consoleMode);

      if (lsStdOutForceStdIO)
      {
        const int32_t handle = _open_osfhandle(reinterpret_cast<intptr_t>(lsStdOutHandle), _O_TEXT);

        if (handle != -1)
          pStdOut = _fdopen(handle, "w");
      }

      // I presume VT Color Codes are in fact slower than `SetConsoleTextAttribute`, since the terminal has to parse those sequences out.
      // Also they're only supported since Windows 10 and would add another code path, therefore they're currently `constexpr` to be `false`.
      //lsStdOutVTCodeColors = (!lsStdOutForceStdIO && SetConsoleMode(lsStdOutHandle, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING));

      constexpr DWORD codepage_UTF8 = 65001;
      SetConsoleOutputCP(codepage_UTF8);
    }
  }
#endif
}

void lsCreateConsole()
{
#ifdef LS_PLATFORM_WINDOWS
  if (GetConsoleWindow() == nullptr)
    if (0 == AllocConsole())
      return;

  lsStdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

  if (lsStdOutHandle == INVALID_HANDLE_VALUE)
    lsStdOutHandle = nullptr;

  if (lsStdOutHandle != nullptr)
  {
    DWORD consoleMode = 0;
    lsStdOutForceStdIO = !GetConsoleMode(lsStdOutHandle, &consoleMode);

    if (lsStdOutForceStdIO)
    {
      const int32_t handle = _open_osfhandle(reinterpret_cast<intptr_t>(lsStdOutHandle), _O_TEXT);

      if (handle != -1)
        pStdOut = _fdopen(handle, "w");
    }

    // I presume VT Color Codes are in fact slower than `SetConsoleTextAttribute`, since the terminal has to parse those sequences out.
    // Also they're only supported since Windows 10 and would add another code path, therefore they're currently `constexpr` to be `false`.
    //lsStdOutVTCodeColors = (!lsStdOutForceStdIO && SetConsoleMode(lsStdOutHandle, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING));

    constexpr DWORD codepage_UTF8 = 65001;
    SetConsoleOutputCP(codepage_UTF8);
  }
#endif
}

#ifdef LS_PLATFORM_WINDOWS
inline WORD lsGetWindowsConsoleColorFromConsoleColor(const lsConsoleColor color)
{
  switch (color & 0xF)
  {
  default:
  case lsCC_Black: return 0;
  case lsCC_DarkBlue: return 1;
  case lsCC_DarkGreen: return 2;
  case lsCC_DarkCyan: return 3;
  case lsCC_DarkRed: return 4;
  case lsCC_DarkMagenta: return 5;
  case lsCC_DarkYellow: return 6;
  case lsCC_BrightGray: return 7;
  case lsCC_DarkGray: return 8;
  case lsCC_BrightBlue: return 9;
  case lsCC_BrightGreen: return 10;
  case lsCC_BrightCyan: return 11;
  case lsCC_BrightRed: return 12;
  case lsCC_BrightMagenta: return 13;
  case lsCC_BrightYellow: return 14;
  case lsCC_White: return 15;
  }
}
#endif

void lsResetConsoleColor()
{
  lsInitConsole();

#ifdef LS_PLATFORM_WINDOWS
  if (lsStdOutHandle != nullptr)
  {
    if (lsStdOutVTCodeColors)
    {
      const char sequence[] = "\x1b[0m";

      WriteConsoleA(lsStdOutHandle, sequence, (DWORD)(LS_ARRAYSIZE(sequence) - 1), nullptr, nullptr);
    }
    else if (!lsStdOutForceStdIO)
    {
      SetConsoleTextAttribute(lsStdOutHandle, lsGetWindowsConsoleColorFromConsoleColor(lsCC_BrightGray) | (lsGetWindowsConsoleColorFromConsoleColor(lsCC_Black) << 8));
    }
  }
#else
  fputs("\x1b[0m", stdout);
#endif
}

void lsSetConsoleColor(const lsConsoleColor foregroundColor, const lsConsoleColor backgroundColor)
{
  lsInitConsole();

#ifdef LS_PLATFORM_WINDOWS
  if (lsStdOutHandle != nullptr)
  {
    if (lsStdOutVTCodeColors)
    {
      const uint8_t fgColor = (foregroundColor & 0xF);
      const uint8_t bgColor = (backgroundColor & 0xF);

      char sequence[64]; // make sure all VT Color Codes _ALWAYS_ fit inside this buffer.
      lsAssert(sformat_capacity("\x1b[", (uint8_t)(fgColor < 0x8 ? (30 + fgColor) : (90 - 8 + fgColor)), ";", (uint8_t)(bgColor < 0x8 ? (40 + bgColor) : (100 - 8 + bgColor)), "m") <= LS_ARRAYSIZE(sequence));
      const bool result = sformat_to(sequence, LS_ARRAYSIZE(sequence), "\x1b[", (uint8_t)(fgColor < 0x8 ? (30 + fgColor) : (90 - 8 + fgColor)), ";", (uint8_t)(bgColor < 0x8 ? (40 + bgColor) : (100 - 8 + bgColor)), "m");
      lsAssert(result);

      WriteConsoleA(lsStdOutHandle, sequence, (DWORD)strnlen(sequence, LS_ARRAYSIZE(sequence)), nullptr, nullptr);
    }
    else if (!lsStdOutForceStdIO)
    {
      const WORD fgColor = lsGetWindowsConsoleColorFromConsoleColor(foregroundColor);
      const WORD bgColor = lsGetWindowsConsoleColorFromConsoleColor(backgroundColor);

      SetConsoleTextAttribute(lsStdOutHandle, fgColor | (bgColor << 4));
    }
  }
#else
  const size_t fgColor = (foregroundColor & 0xF);
  const size_t bgColor = (backgroundColor & 0xF);

  printf("\x1b[%" PRIu64 ";%" PRIu64 "m", fgColor < 0x8 ? (30 + fgColor) : (90 - 8 + fgColor), bgColor < 0x8 ? (40 + bgColor) : (100 - 8 + bgColor));
#endif
}

#ifdef LS_PLATFORM_WINDOWS
void lsPrintToOutputWithLength(const char *text, const size_t length)
{
  lsInitConsole();

  if (lsStdOutHandle != nullptr)
  {
    if (!lsStdOutForceStdIO)
      WriteConsoleA(lsStdOutHandle, text, (DWORD)length, nullptr, nullptr);
    else if (pStdOut != nullptr)
      fputs(text, pStdOut);
  }

  if (lsFileOutHandle != nullptr)
    WriteFile(lsFileOutHandle, text, (DWORD)length, nullptr, nullptr);
}
#endif

void lsPrintToOutput(const char *text)
{
  if (text == nullptr)
    return;

#ifdef LS_PLATFORM_WINDOWS
  lsPrintToOutputWithLength(text, strlen(text));
#else
  fputs(text, stdout);
#endif
}

void lsDefaultPrint(const char *text)
{
  lsPrintToOutput(text);

#ifndef RELEASE_BUILD
  print_to_dbgcon(text);
#endif
}

void lsLogPrint(const char *text)
{
  lsSetConsoleColor(lsCC_White, lsCC_Black);
  lsPrintToOutputArray("[Log]   ");
  lsSetConsoleColor(lsCC_DarkCyan, lsCC_Black);
  lsPrintToOutput(text);
  lsResetConsoleColor();
  lsPrintToOutputArray("\n");

#ifndef RELEASE_BUILD
  print_to_dbgcon("[Log]   ");
  print_to_dbgcon(text);
  print_to_dbgcon("\n");
#endif
}

void lsErrorPrint(const char *text)
{
  lsSetConsoleColor(lsCC_BrightRed, lsCC_Black);
  lsPrintToOutputArray("[Error] ");
  lsSetConsoleColor(lsCC_BrightYellow, lsCC_DarkRed);
  lsPrintToOutput(text);
  lsResetConsoleColor();
  lsPrintToOutputArray("\n");

#ifndef RELEASE_BUILD
  print_to_dbgcon("[Error] ");
  print_to_dbgcon(text);
  print_to_dbgcon("\n");
#endif
}

#ifdef LS_PLATFORM_WINDOWS
lsResult lsSetOutputFilePath(const char *path, const bool append /* = true */)
{
  lsResult result = lsR_Success;
  HANDLE file = nullptr;

  wchar_t wpath[MAX_PATH];
  LS_ERROR_CHECK(lsToWide(path, wpath, LS_ARRAYSIZE(wpath)));

  file = CreateFileW(wpath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  LS_ERROR_IF(file == nullptr || file == INVALID_HANDLE_VALUE, lsR_IOFailure);

  if (GetLastError() == ERROR_ALREADY_EXISTS && append)
    SetFilePointer(file, 0, nullptr, FILE_END);

  lsResetOutputFile();

  lsFileOutHandle = file;

epilogue:
  if (LS_FAILED(result))
    CloseHandle(file);

  return result;
}

void lsResetOutputFile()
{
  if (lsFileOutHandle == nullptr)
    return;

  HANDLE file = lsFileOutHandle;
  lsFileOutHandle = nullptr;

  FlushFileBuffers(file);
  CloseHandle(file);
}
#endif

void lsFlushOutput()
{
#ifdef LS_PLATFORM_WINDOWS
  if (lsFileOutHandle != nullptr)
    FlushFileBuffers(lsFileOutHandle);
#endif
}

lsPrintCallbackFunc *lsPrintCallback = &lsDefaultPrint;
lsPrintCallbackFunc *lsPrintErrorCallback = &lsErrorPrint;
lsPrintCallbackFunc *lsPrintLogCallback = &lsLogPrint;

void print_to_dbgcon(const char *text)
{
#ifdef LS_PLATFORM_WINDOWS
  if (text != nullptr && text[0] != '\0')
    OutputDebugStringA(text);
#else
  (void)text;
#endif
}

#ifdef LS_PLATFORM_WINDOWS
lsResult lsToWide(_In_ const char *string, const size_t inChars, _Out_ wchar_t *out, const size_t capacity, _Out_ size_t &writtenChars)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(out == nullptr, lsR_ArgumentNull);

  if (string == nullptr)
  {
    LS_ERROR_IF(capacity == 0, lsR_ArgumentOutOfBounds);

    out[0] = L'\0';
    writtenChars = 1;
  }
  else
  {
    int32_t length = 0;
    LS_ERROR_IF(inChars > INT32_MAX, lsR_ArgumentOutOfBounds);

    if (0 >= (length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, string, (int32_t)inChars, out, (int32_t)lsMin(INT32_MAX, capacity))))
    {
      const DWORD error = GetLastError();

      switch (error)
      {
      case ERROR_INSUFFICIENT_BUFFER:
        LS_ERROR_SET(lsR_ArgumentOutOfBounds);

      case ERROR_NO_UNICODE_TRANSLATION:
        LS_ERROR_SET(lsR_InvalidParameter);

      case ERROR_INVALID_FLAGS:
      case ERROR_INVALID_PARAMETER:
      default:
        LS_ERROR_SET(lsR_InternalError);
      }
    }

    writtenChars = length;
  }

epilogue:
  return result;
}
#endif
