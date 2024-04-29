#include "io.h"

//////////////////////////////////////////////////////////////////////////

constexpr bool LogIO = true;
#define IOLogPrefix "[ I/O Subsystem  ] "

//////////////////////////////////////////////////////////////////////////

lsResult lsReadFileBytes(const char *filename, uint8_t **ppData, const size_t elementSize, size_t *pCount)
{
  lsResult result = lsR_Success;

  FILE *pFile = nullptr;
  size_t length, readLength;

  LS_ERROR_IF(filename == nullptr || ppData == nullptr || pCount == nullptr, lsR_ArgumentNull);

  pFile = fopen(filename, "rb");
  
  if constexpr (LogIO)
    if (pFile == nullptr)
      print_error_line(IOLogPrefix "Failed to open file: '", filename , "' with read access.");

  LS_ERROR_IF(pFile == nullptr, lsR_ResourceNotFound);

  LS_ERROR_IF(0 != fseek(pFile, 0, SEEK_END), lsR_IOFailure);

  length = ftell(pFile);

  LS_ERROR_IF(0 != fseek(pFile, 0, SEEK_SET), lsR_IOFailure);

  LS_ERROR_CHECK(lsAlloc(ppData, length + (elementSize > 2 ? 0 : elementSize)));

  if (elementSize <= 2)
    lsZeroMemory(&((*ppData)[length]), elementSize); // To zero terminate strings. This is out of bounds for all other data types anyways.

  readLength = fread(*ppData, 1, length, pFile);

  *pCount = readLength / elementSize;

epilogue:
  if (pFile != nullptr)
    fclose(pFile);

  return result;
}

lsResult lsReadFileBytesSized(const char *filename, _Out_ uint8_t *pData, const size_t elementSize, const size_t count)
{
  lsResult result = lsR_Success;

  const size_t requestedBytes = elementSize * count;
  FILE *pFile = nullptr;
  size_t length, readLength;

  LS_ERROR_IF(filename == nullptr || pData == nullptr, lsR_ArgumentNull);

  pFile = fopen(filename, "rb");
  
  if constexpr (LogIO)
    if (pFile == nullptr)
      print_error_line(IOLogPrefix "Failed to open file: '", filename, "' with write access.");

  LS_ERROR_IF(pFile == nullptr, lsR_ResourceNotFound);

  LS_ERROR_IF(0 != fseek(pFile, 0, SEEK_END), lsR_IOFailure);

  length = ftell(pFile);
  LS_ERROR_IF(length < requestedBytes, lsR_EndOfStream);

  LS_ERROR_IF(0 != fseek(pFile, 0, SEEK_SET), lsR_IOFailure);

  readLength = fread(pData, 1, requestedBytes, pFile);

  LS_ERROR_IF(readLength != requestedBytes, lsR_IOFailure);

epilogue:
  if (pFile != nullptr)
    fclose(pFile);

  return result;
}

lsResult lsWriteFileBytes(const char *filename, const uint8_t *pData, const size_t size)
{
  lsResult result = lsR_Success;

  FILE *pFile = nullptr;

  LS_ERROR_IF(filename == nullptr || pData == nullptr, lsR_ArgumentNull);

  pFile = fopen(filename, "wb");
  LS_ERROR_IF(pFile == nullptr, lsR_IOFailure);

  LS_ERROR_IF(size != fwrite(pData, 1, size, pFile), lsR_IOFailure);

epilogue:
  if (pFile != nullptr)
    fclose(pFile);

  return result;
}
