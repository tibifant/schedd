#pragma once

#include "core.h"

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t multiBlockAllocCount>
struct pool;

template <typename T, size_t multiBlockAllocCount>
struct pool_iterator
{
  pool<T, multiBlockAllocCount> *pPool = nullptr;
  size_t blockIndex = 0, blockSubIndex = 0, iteratedItem = 0;

  struct pool_item
  {
    size_t index;
    T *pItem;
    size_t _iteratedIndex;
  };

  pool_iterator(pool<T, multiBlockAllocCount> *pPool);
  pool_item operator *();
  const pool_item operator *() const;
  bool operator != (const size_t maxCount) const;
  pool_iterator &operator++();
};

template <typename T, size_t multiBlockAllocCount>
struct pool_const_iterator
{
  const pool<T, multiBlockAllocCount> *pPool = nullptr;
  size_t blockIndex = 0, blockSubIndex = 0, iteratedItem = 0;

  struct pool_item
  {
    size_t index;
    const T *pItem;
    size_t _iteratedIndex;
  };

  pool_const_iterator(const pool<T, multiBlockAllocCount> *pPool);
  const pool_item operator *() const;
  bool operator != (const size_t maxCount) const;
  pool_const_iterator &operator++();
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct _pool_block_alloc_count
{
  static const size_t value = lsClamp((1024 * 16) / (sizeof(T) * 64), (size_t)1, (size_t)8);
};

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t multiBlockAllocCount = _pool_block_alloc_count<T>::value>
struct pool
{
  size_t count = 0;
  size_t blockCount = 0;
  uint64_t *pBlockEmptyMask = nullptr;
  T **ppBlocks = nullptr;

  static constexpr size_t BlockSize = sizeof(uint64_t) * CHAR_BIT;

  inline pool_iterator<T, multiBlockAllocCount> begin() { return pool_iterator(this); };
  inline pool_const_iterator<T, multiBlockAllocCount> begin() const { return pool_const_iterator(this); };
  inline const size_t & end() { return count; };
  inline const size_t & end() const { return count; };

  inline struct
  {
    pool<T, multiBlockAllocCount> *pPool;
    size_t poolIndex, iteratedIndex;

    inline pool_iterator<T, multiBlockAllocCount> begin()
    {
      auto it = pool_iterator(pPool);

      it.iteratedItem = iteratedIndex;
      it.blockIndex = poolIndex / pool<T, multiBlockAllocCount>::BlockSize;
      it.blockSubIndex = poolIndex % pool<T, multiBlockAllocCount>::BlockSize;
      
      return it;
    };
    
    inline const size_t & end() { return pPool->count; };

  } IterateFromIteratedIndex(const size_t poolIndex, const size_t iteratedIndex) { return { this, poolIndex, iteratedIndex }; }

  inline pool() {};
  inline pool(const pool &) = delete;
  pool & operator = (const pool &) = delete;
  
  inline pool(pool &&move) :
    count(move.count),
    blockCount(move.blockCount),
    pBlockEmptyMask(move.pBlockEmptyMask),
    ppBlocks(move.ppBlocks)
  {
    move.count = 0;
    move.blockCount = 0;
    move.ppBlocks = nullptr;
    move.pBlockEmptyMask = nullptr;
  }

  pool &operator = (pool &&move)
  {
    pool_destroy(this);

    count = move.count;
    blockCount = move.blockCount;
    pBlockEmptyMask = move.pBlockEmptyMask;
    ppBlocks = move.ppBlocks;

    move.count = 0;
    move.blockCount = 0;
    move.ppBlocks = nullptr;
    move.pBlockEmptyMask = nullptr;

    return *this;
  }

  ~pool();
};

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t multiBlockAllocCount>
lsResult pool_reserve_blocks(pool<T, multiBlockAllocCount> *pPool, const size_t blockCount)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pPool == nullptr, lsR_ArgumentNull);

  if (pPool->blockCount < blockCount)
  {
    const size_t newSize = ((blockCount + multiBlockAllocCount - 1) / multiBlockAllocCount) * multiBlockAllocCount;
    LS_ERROR_CHECK(lsRealloc(&pPool->pBlockEmptyMask, newSize));
    LS_ERROR_CHECK(lsRealloc(&pPool->ppBlocks, newSize));

    while (pPool->blockCount < blockCount)
    {
      const size_t newBlockCount = pPool->blockCount + multiBlockAllocCount;

      LS_ERROR_CHECK(lsAllocZero(&pPool->ppBlocks[pPool->blockCount], pool<T, multiBlockAllocCount>::BlockSize * multiBlockAllocCount));

      for (size_t i = pPool->blockCount; i < newBlockCount; i++)
        pPool->pBlockEmptyMask[i] = 0;

      if constexpr (multiBlockAllocCount > 1)
        for (size_t i = pPool->blockCount + 1; i < newBlockCount; i++)
          pPool->ppBlocks[i] = pPool->ppBlocks[i - 1] + pool<T, multiBlockAllocCount>::BlockSize;

      pPool->blockCount = newBlockCount;
    }

    lsAssert(newSize == pPool->blockCount);
  }

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_reserve(pool<T, multiBlockAllocCount> *pPool, const size_t count)
{
  if (count == 0)
    return lsR_Success;

  const size_t blockIndex = (count - 1) / pool<T, multiBlockAllocCount>::BlockSize;
  
  return pool_reserve_blocks(pPool, blockIndex + 1);
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_allocate(pool<T, multiBlockAllocCount> *pPool, T **ppItem, _Out_ size_t *pIndex)
{
  lsResult result = lsR_Success;

  bool found = false;
  size_t blockIndex = 0, blockSubIndex = 0;

  LS_ERROR_IF(pPool == nullptr || ppItem == nullptr || pIndex == nullptr, lsR_ArgumentNull);

  // Try to find an empty spot.
  for (size_t i = 0; i < pPool->blockCount; i++)
  {
    if (pPool->pBlockEmptyMask[i] != (uint64_t)-1)
    {
      unsigned long subIndex = 0;
      lsAssert(0 != _BitScanForward64(&subIndex, ~pPool->pBlockEmptyMask[i]));
      lsAssert(((pPool->pBlockEmptyMask[i] >> subIndex) & 1) == 0);

      blockIndex = i;
      blockSubIndex = subIndex;
      found = true;

      break;
    }
  }

  // Spot found? No? Then allocate a new block!
  if (!found)
  {
    const size_t newBlockCount = pPool->blockCount + multiBlockAllocCount;

    blockIndex = pPool->blockCount;
    blockSubIndex = 0;

    LS_ERROR_CHECK(lsRealloc(&pPool->pBlockEmptyMask, newBlockCount));
    LS_ERROR_CHECK(lsRealloc(&pPool->ppBlocks, newBlockCount));
    LS_ERROR_CHECK(lsAllocZero(&pPool->ppBlocks[blockIndex], pool<T, multiBlockAllocCount>::BlockSize * multiBlockAllocCount));

    for (size_t i = blockIndex; i < newBlockCount; i++)
      pPool->pBlockEmptyMask[i] = 0;
    
    if constexpr (multiBlockAllocCount > 1)
      for (size_t i = blockIndex + 1; i < newBlockCount; i++)
        pPool->ppBlocks[i] = pPool->ppBlocks[i - 1] + pool<T, multiBlockAllocCount>::BlockSize;

    pPool->blockCount = newBlockCount;
  }

  *ppItem = &pPool->ppBlocks[blockIndex][blockSubIndex];
  pPool->pBlockEmptyMask[blockIndex] |= ((uint64_t)1 << blockSubIndex);
  *pIndex = blockIndex * pool<T, multiBlockAllocCount>::BlockSize + blockSubIndex;
  pPool->count++;

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_add(pool<T, multiBlockAllocCount> *pPool, const T *pItem, _Out_ size_t *pIndex)
{
  lsResult result = lsR_Success;
  T *pDst = nullptr;

  LS_ERROR_IF(pItem == nullptr, lsR_ArgumentNull);
  LS_ERROR_CHECK(pool_allocate(pPool, &pDst, pIndex));

  *pDst = *pItem;

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_add(pool<T, multiBlockAllocCount> *pPool, T &&item, _Out_ size_t *pIndex)
{
  lsResult result = lsR_Success;

  T *pDst = nullptr;
  LS_ERROR_CHECK(pool_allocate(pPool, &pDst, pIndex));

  new (pDst) T(std::move(item));

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_add(pool<T, multiBlockAllocCount> *pPool, const T &item, _Out_ size_t *pIndex)
{
  return pool_add(pPool, &item, pIndex);
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_insertAt(pool<T, multiBlockAllocCount> *pPool, const T *pItem, const size_t index, const bool allowOverride = false)
{
  lsResult result = lsR_Success;

  const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
  const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

  LS_ERROR_IF(pPool == nullptr || pItem == nullptr, lsR_ArgumentNull);

  LS_ERROR_CHECK(pool_reserve_blocks(pPool, blockIndex + 1));

  {
    const bool isOverride = pPool->pBlockEmptyMask[blockIndex] & ((uint64_t)1 << blockSubIndex);
    LS_ERROR_IF(isOverride && !allowOverride, lsR_ResourceAlreadyExists);

    pPool->ppBlocks[blockIndex][blockSubIndex] = *pItem;
    pPool->pBlockEmptyMask[blockIndex] |= ((uint64_t)1 << blockSubIndex);

    if (!isOverride)
      pPool->count++;
  }

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_insertAt(pool<T, multiBlockAllocCount> *pPool, const T &item, const size_t index, const bool allowOverride = false)
{
  return pool_insertAt(pPool, &item, index, allowOverride);
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_insertAt(pool<T, multiBlockAllocCount> *pPool, T &&item, const size_t index, const bool allowOverride = false)
{
  lsResult result = lsR_Success;

  const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
  const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

  LS_ERROR_IF(pPool == nullptr, lsR_ArgumentNull);

  LS_ERROR_CHECK(pool_reserve_blocks(pPool, blockIndex + 1));

  {
    const bool isOverride = pPool->pBlockEmptyMask[blockIndex] & ((uint64_t)1 << blockSubIndex);
    LS_ERROR_IF(isOverride && !allowOverride, lsR_ResourceAlreadyExists);

    if (!isOverride)
      new (&pPool->ppBlocks[blockIndex][blockSubIndex]) T(std::move(item));
    else
      pPool->ppBlocks[blockIndex][blockSubIndex] = std::move(item);

    pPool->pBlockEmptyMask[blockIndex] |= ((uint64_t)1 << blockSubIndex);

    if (!isOverride)
      pPool->count++;
  }

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_insertIfNotContainedAndRetrieve(pool<T, multiBlockAllocCount> *pPool, const T *pItem, const size_t index, T **pOut, OPTIONAL OUT bool *pExisted = nullptr)
{
  lsResult result = lsR_Success;

  const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
  const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

  LS_ERROR_IF(pPool == nullptr || pItem == nullptr, lsR_ArgumentNull);

  LS_ERROR_CHECK(pool_reserve_blocks(pPool, blockIndex + 1));

  {
    const bool exists = pPool->pBlockEmptyMask[blockIndex] & ((uint64_t)1 << blockSubIndex);

    if (!exists)
    {
      pPool->ppBlocks[blockIndex][blockSubIndex] = *pItem;
      pPool->pBlockEmptyMask[blockIndex] |= ((uint64_t)1 << blockSubIndex);
      pPool->count++;
    }

    if (pExisted != nullptr)
      *pExisted = exists;

    *pOut = &pPool->ppBlocks[blockIndex][blockSubIndex];
  }

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
T * pool_get(pool<T, multiBlockAllocCount> *pPool, const size_t index)
{
  const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
  const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

  lsAssert(pPool->blockCount > blockIndex);
  lsAssert((pPool->pBlockEmptyMask[blockIndex] >> blockSubIndex) & 1);

  return &pPool->ppBlocks[blockIndex][blockSubIndex];
}

template <typename T, size_t multiBlockAllocCount>
const T * pool_get(const pool<T, multiBlockAllocCount> *pPool, const size_t index)
{
  const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
  const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

  lsAssert(pPool->blockCount > blockIndex);
  lsAssert((pPool->pBlockEmptyMask[blockIndex] >> blockSubIndex) & 1);

  return &pPool->ppBlocks[blockIndex][blockSubIndex];
}

template <typename T, size_t multiBlockAllocCount>
T * pool_get(pool<T, multiBlockAllocCount> &p, const size_t index)
{
  const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
  const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

  lsAssert(p.blockCount > blockIndex);
  lsAssert((p.pBlockEmptyMask[blockIndex] >> blockSubIndex) & 1);

  return &p.ppBlocks[blockIndex][blockSubIndex];
}

template <typename T, size_t multiBlockAllocCount>
const T * pool_get(const pool<T, multiBlockAllocCount> &p, const size_t index)
{
  const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
  const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

  lsAssert(p.blockCount > blockIndex);
  lsAssert((p.pBlockEmptyMask[blockIndex] >> blockSubIndex) & 1);

  return &p.ppBlocks[blockIndex][blockSubIndex];
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_get_safe(const pool<T, multiBlockAllocCount> *pPool, const size_t index, _Out_ T *pItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pPool == nullptr || pItem == nullptr, lsR_ArgumentNull);

  {
    const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
    const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

    LS_ERROR_IF(pPool->blockCount <= blockIndex, lsR_ResourceNotFound);
    LS_ERROR_IF((pPool->pBlockEmptyMask[blockIndex] & ((uint64_t)1 << blockSubIndex)) == 0, lsR_ResourceNotFound);

    *pItem = pPool->ppBlocks[blockIndex][blockSubIndex];
  }

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_get_safe(pool<T, multiBlockAllocCount> *pPool, const size_t index, _Out_ T **ppItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pPool == nullptr || ppItem == nullptr, lsR_ArgumentNull);

  {
    const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
    const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

    LS_ERROR_IF(pPool->blockCount <= blockIndex, lsR_ResourceNotFound);
    LS_ERROR_IF((pPool->pBlockEmptyMask[blockIndex] & ((uint64_t)1 << blockSubIndex)) == 0, lsR_ResourceNotFound);

    *ppItem = &pPool->ppBlocks[blockIndex][blockSubIndex];
  }

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_get_safe(const pool<T, multiBlockAllocCount> *pPool, const size_t index, _Out_ T * const *ppItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pPool == nullptr || ppItem == nullptr, lsR_ArgumentNull);

  {
    const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
    const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

    LS_ERROR_IF(pPool->blockCount <= blockIndex, lsR_ResourceNotFound);
    LS_ERROR_IF((pPool->pBlockEmptyMask[blockIndex] & ((uint64_t)1 << blockSubIndex)) == 0, lsR_ResourceNotFound);

    *ppItem = &pPool->ppBlocks[blockIndex][blockSubIndex];
  }

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
void pool_prefetch(const pool<T, multiBlockAllocCount> *pPool, const size_t index)
{
  const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
  const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

  lsAssert(pPool->blockCount > blockIndex);

  _mm_prefetch(reinterpret_cast<const char *>(&pPool->ppBlocks[blockIndex][blockSubIndex]), 1);
}

template <typename T, size_t multiBlockAllocCount>
lsResult pool_remove_safe(pool<T, multiBlockAllocCount> *pPool, const size_t index, _Out_ T *pItem = nullptr)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pPool == nullptr, lsR_ArgumentNull);

  {
    const size_t blockIndex = index / pool<T, multiBlockAllocCount>::BlockSize;
    const size_t blockSubIndex = index % pool<T, multiBlockAllocCount>::BlockSize;

    LS_ERROR_IF(pPool->blockCount <= blockIndex, lsR_ResourceNotFound);
    LS_ERROR_IF((pPool->pBlockEmptyMask[blockIndex] & ((uint64_t)1 << blockSubIndex)) == 0, lsR_ResourceNotFound);

    if (pItem != nullptr)
      *pItem = std::move(pPool->ppBlocks[blockIndex][blockSubIndex]);
    else
      pPool->ppBlocks[blockIndex][blockSubIndex].~T();

    pPool->pBlockEmptyMask[blockIndex] &= ~(uint64_t)((uint64_t)1 << blockSubIndex);
    pPool->count--;
  }

epilogue:
  return result;
}

template <typename T, size_t multiBlockAllocCount>
void pool_clear(pool<T, multiBlockAllocCount> *pPool)
{
  if (pPool == nullptr)
    return;

  for (auto &&_item : *pPool)
    _item.pItem->~T();

  for (size_t i = 0; i < pPool->blockCount; i++)
    pPool->pBlockEmptyMask[i] = 0;

  pPool->count = 0;
}

template <typename T, size_t multiBlockAllocCount>
void pool_destroy(pool<T, multiBlockAllocCount> *pPool)
{
  if (pPool == nullptr)
    return;

  for (auto &&_item : *pPool)
    _item.pItem->~T();

  for (size_t i = 0; i < pPool->blockCount; i += multiBlockAllocCount)
    lsFreePtr(&pPool->ppBlocks[i]);

  lsFreePtr(&pPool->ppBlocks);
  lsFreePtr(&pPool->pBlockEmptyMask);

  pPool->blockCount = 0;
  pPool->count = 0;
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t multiBlockAllocCount>
inline pool_iterator<T, multiBlockAllocCount>::pool_iterator(pool<T, multiBlockAllocCount> *pPool) :
  pPool(pPool)
{
  if (pPool == nullptr || pPool->count == 0)
    return;

  while (true)
  {
    if (pPool->pBlockEmptyMask[blockIndex] == 0)
    {
      blockIndex++;

      lsAssert(blockIndex < pPool->blockCount);
    }
    else
    {
      unsigned long subIndex = 0;

      if (0 == _BitScanForward64(&subIndex, pPool->pBlockEmptyMask[blockIndex]))
      {
        blockIndex++;
        blockSubIndex = 0;
        lsAssert(blockIndex < pPool->blockCount);
      }
      else
      {
        blockSubIndex += subIndex;
        lsAssert((blockSubIndex < pool<T, multiBlockAllocCount>::BlockSize));
        break;
      }
    }
  }
}

template<typename T, size_t multiBlockAllocCount>
inline typename pool_iterator<T, multiBlockAllocCount>::pool_item pool_iterator<T, multiBlockAllocCount>::operator*()
{
  pool_iterator<T, multiBlockAllocCount>::pool_item ret;
  ret.index = blockIndex * pool<T, multiBlockAllocCount>::BlockSize + blockSubIndex;
  ret.pItem = &pPool->ppBlocks[blockIndex][blockSubIndex];
  ret._iteratedIndex = iteratedItem;

  return ret;
}

template<typename T, size_t multiBlockAllocCount>
inline const typename pool_iterator<T, multiBlockAllocCount>::pool_item pool_iterator<T, multiBlockAllocCount>::operator*() const
{
  pool_iterator<T, multiBlockAllocCount>::pool_item ret;
  ret.index = blockIndex * pool<T, multiBlockAllocCount>::BlockSize + blockSubIndex;
  ret.pItem = &pPool->ppBlocks[blockIndex][blockSubIndex];
  ret._iteratedIndex = iteratedItem;
  
  return ret;
}

template<typename T, size_t multiBlockAllocCount>
inline bool pool_iterator<T, multiBlockAllocCount>::operator!=(const size_t maxCount) const
{
  return iteratedItem < maxCount;
}

template<typename T, size_t multiBlockAllocCount>
inline pool_iterator<T, multiBlockAllocCount> &pool_iterator<T, multiBlockAllocCount>::operator++()
{
  if (iteratedItem + 1 < pPool->count)
  {
    blockSubIndex++;

    while (true)
    {
      if (pPool->pBlockEmptyMask[blockIndex] == 0)
      {
        blockIndex++;

        lsAssert(blockIndex < pPool->blockCount);
      }
      else
      {
        unsigned long subIndex = 0;
        const uint64_t shift = blockSubIndex;

        if (shift >= 0x40 || 0 == _BitScanForward64(&subIndex, pPool->pBlockEmptyMask[blockIndex] >> shift))
        {
          blockIndex++;
          blockSubIndex = 0;
          lsAssert(blockIndex < pPool->blockCount);
        }
        else
        {
          blockSubIndex += subIndex;
          lsAssert((blockSubIndex < pool<T, multiBlockAllocCount>::BlockSize));
          break;
        }
      }
    }
  }

  iteratedItem++;

  return *this;
}

template<typename T, size_t multiBlockAllocCount>
inline pool_const_iterator<T, multiBlockAllocCount>::pool_const_iterator(const pool<T, multiBlockAllocCount> *pPool) :
  pPool(pPool)
{
  if (pPool == nullptr || pPool->count == 0)
    return;

  while (true)
  {
    if (pPool->pBlockEmptyMask[blockIndex] == 0)
    {
      blockIndex++;

      lsAssert(blockIndex < pPool->blockCount);
    }
    else
    {
      unsigned long subIndex = 0;

      if (0 == _BitScanForward64(&subIndex, pPool->pBlockEmptyMask[blockIndex]))
      {
        blockIndex++;
        blockSubIndex = 0;
        lsAssert(blockIndex < pPool->blockCount);
      }
      else
      {
        blockSubIndex += subIndex;
        lsAssert((blockSubIndex < pool<T, multiBlockAllocCount>::BlockSize));
        break;
      }
    }
  }
}

template<typename T, size_t multiBlockAllocCount>
inline const typename pool_const_iterator<T, multiBlockAllocCount>::pool_item pool_const_iterator<T, multiBlockAllocCount>::operator*() const
{
  pool_const_iterator<T, multiBlockAllocCount>::pool_item ret;
  ret.index = blockIndex * pool<T, multiBlockAllocCount>::BlockSize + blockSubIndex;
  ret.pItem = &pPool->ppBlocks[blockIndex][blockSubIndex];
  ret._iteratedIndex = iteratedItem;

  return ret;
}

template<typename T, size_t multiBlockAllocCount>
inline bool pool_const_iterator<T, multiBlockAllocCount>::operator!=(const size_t maxCount) const
{
  return iteratedItem < maxCount;
}

template<typename T, size_t multiBlockAllocCount>
inline pool_const_iterator<T, multiBlockAllocCount> &pool_const_iterator<T, multiBlockAllocCount>::operator++()
{
  if (iteratedItem + 1 < pPool->count)
  {
    blockSubIndex++;

    while (true)
    {
      if (pPool->pBlockEmptyMask[blockIndex] == 0)
      {
        blockIndex++;

        lsAssert(blockIndex < pPool->blockCount);
      }
      else
      {
        unsigned long subIndex = 0;
        const uint64_t shift = blockSubIndex;

        if (shift >= 0x40 || 0 == _BitScanForward64(&subIndex, pPool->pBlockEmptyMask[blockIndex] >> shift))
        {
          blockIndex++;
          blockSubIndex = 0;
          lsAssert(blockIndex < pPool->blockCount);
        }
        else
        {
          blockSubIndex += subIndex;
          lsAssert((blockSubIndex < pool<T, multiBlockAllocCount>::BlockSize));
          break;
        }
      }
    }
  }

  iteratedItem++;

  return *this;
}

template<typename T, size_t multiBlockAllocCount>
inline pool<T, multiBlockAllocCount>::~pool()
{
  pool_destroy(this);
}
