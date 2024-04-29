#pragma once

#include "core.h"

lsResult lsReadFileBytes(const char *filename, _Out_ uint8_t **ppData, const size_t elementSize, _Out_ size_t *pCount);
lsResult lsReadFileBytesSized(const char *filename, _Out_ uint8_t *pData, const size_t elementSize, const size_t count);

template <typename T>
lsResult lsReadFile(const char *filename, _Out_ T **ppData, _Out_ size_t *pCount)
{
  return lsReadFileBytes(filename, reinterpret_cast<uint8_t **>(ppData), sizeof(T), pCount);
}

template <typename T>
lsResult lsReadFileSized(const char *filename, _Out_ T *pData, const size_t count)
{
  return lsReadFileBytesSized(filename, reinterpret_cast<uint8_t *>(pData), sizeof(T), count);
}

lsResult lsWriteFileBytes(const char *filename, const uint8_t *pData, const size_t size);

template <typename T>
lsResult lsWriteFile(const char *filename, const T *pData, const size_t count)
{
  return lsWriteFileBytes(filename, reinterpret_cast<const uint8_t *>(pData), count * sizeof(T));
}
