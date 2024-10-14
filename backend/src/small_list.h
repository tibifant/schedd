#pragma once

#include "core.h"

template <typename T, size_t internal_count>
struct small_list;

template <typename T, size_t internal_count>
struct small_list_reverse_iterator;

template <typename T, size_t internal_count>
struct small_list_const_iterator;

template <typename T, size_t internal_count>
struct small_list_const_reverse_iterator;

template <typename T, size_t internal_count>
struct small_list_iterator
{
  small_list<T, internal_count> *pList = nullptr;
  bool inInternal = true;
  size_t subIndex = 0;
  size_t position = 0;

  small_list_iterator(small_list<T, internal_count> *pList);
  T &operator *();
  const T &operator *() const;
  bool operator != (const size_t maxCount) const;
  bool operator != (const small_list_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_reverse_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_const_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_const_reverse_iterator<T, internal_count> &it) const;
  small_list_iterator<T, internal_count> &operator++();
};

template <typename T, size_t internal_count>
struct small_list_reverse_iterator
{
  small_list<T, internal_count> *pList = nullptr;
  bool inInternal = false;
  int64_t subIndex = 0;
  int64_t position = 0;

  small_list_reverse_iterator(small_list<T, internal_count> *pList);
  T &operator *();
  const T &operator *() const;
  bool operator != (const size_t startIndex) const;
  bool operator != (const small_list_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_reverse_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_const_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_const_reverse_iterator<T, internal_count> &it) const;
  small_list_reverse_iterator<T, internal_count> &operator++();
};

template <typename T, size_t internal_count>
struct small_list_const_iterator
{
  const small_list<T, internal_count> *pList = nullptr;
  bool inInternal = true;
  size_t subIndex = 0;
  size_t position = 0;

  small_list_const_iterator(const small_list<T, internal_count> *pList);
  const T &operator *() const;
  bool operator != (const size_t maxCount) const;
  bool operator != (const small_list_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_reverse_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_const_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_const_reverse_iterator<T, internal_count> &it) const;
  small_list_const_iterator<T, internal_count> &operator++();
};

template <typename T, size_t internal_count>
struct small_list_const_reverse_iterator
{
  const small_list<T, internal_count> *pList = nullptr;
  bool inInternal = false;
  int64_t subIndex = 0;
  int64_t position = 0;

  small_list_const_reverse_iterator(const small_list<T, internal_count> *pList);
  const T &operator *() const;
  bool operator != (const size_t startIndex) const;
  bool operator != (const small_list_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_reverse_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_const_iterator<T, internal_count> &it) const;
  bool operator != (const small_list_const_reverse_iterator<T, internal_count> &it) const;
  small_list_const_reverse_iterator<T, internal_count> &operator++();
};

//////////////////////////////////////////////////////////////////////////

template <size_t a, size_t b>
struct _small_list_max
{
  static const size_t value = a > b ? a : b;
};

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count = _small_list_max<16, 128 / sizeof(T)>::value>
struct small_list
{
  T values[internal_count];
  size_t count = 0;
  T *pExtra = nullptr;
  size_t extraCapacity = 0;

  inline small_list_iterator<T, internal_count> begin() { return small_list_iterator<T, internal_count>(this); };
  inline small_list_const_iterator<T, internal_count> begin() const { return small_list_const_iterator<T, internal_count>(this); };
  inline size_t end() const { return count; };

  ~small_list();

  struct IterateReverseWrapper
  {
    small_list<T, internal_count> *pList;
    inline small_list_reverse_iterator<T, internal_count> begin() { return small_list_reverse_iterator<T, internal_count>(pList); };
    inline size_t end() { return 0; };
  };

  inline IterateReverseWrapper IterateReverse() { return { this }; };

  struct IterateFromWrapper
  {
    small_list<T, internal_count> *pList;
    size_t startIdx;

    inline IterateFromWrapper(small_list<T, internal_count> *pList, size_t startIdx) : pList(pList), startIdx(startIdx) {}

    inline small_list_iterator<T, internal_count> begin()
    {
      small_list_iterator<T, internal_count> it = small_list_iterator<T, internal_count>(pList);

      it.inInternal = startIdx < internal_count;
      it.position = startIdx;
      it.subIndex = it.inInternal ? startIdx : startIdx - internal_count;

      return it;
    };

    inline size_t end() { return pList->count; };
  };

  inline IterateFromWrapper IterateFrom(const size_t idx) { return IterateFromWrapper(this, idx); };

  struct ConstIterateFromWrapper
  {
    const small_list<T, internal_count> *pList;
    size_t startIdx;

    inline ConstIterateFromWrapper(const small_list<T, internal_count> *pList, size_t startIdx) : pList(pList), startIdx(startIdx) {}

    inline small_list_const_iterator<T, internal_count> begin()
    {
      small_list_const_iterator<T, internal_count> it = small_list_const_iterator<T, internal_count>(pList);

      it.inInternal = startIdx < internal_count;
      it.position = startIdx;
      it.subIndex = it.inInternal ? startIdx : startIdx - internal_count;

      return it;
    };

    inline size_t end() { return pList->count; };
  };

  inline ConstIterateFromWrapper IterateFrom(const size_t idx) const { return ConstIterateFromWrapper(this, idx); };

  struct IterateReverseFromWrapper
  {
    small_list<T, internal_count> *pList;
    size_t startIdx;

    inline IterateReverseFromWrapper(small_list<T, internal_count> *pList, size_t startIdx) : pList(pList), startIdx(startIdx) {}

    inline small_list_reverse_iterator<T, internal_count> begin()
    {
      small_list_reverse_iterator<T, internal_count> it = small_list_reverse_iterator<T, internal_count>(pList);

      it.inInternal = startIdx < internal_count;
      it.position = startIdx;
      it.subIndex = it.inInternal ? startIdx : startIdx - internal_count;

      return it;
    };

    inline size_t end() { return 0; };
  };

  inline IterateReverseFromWrapper IterateReverseFrom(const size_t idx) { return IterateReverseFromWrapper(this, idx); };

  struct ConstIterateReverseFromWrapper
  {
    const small_list<T, internal_count> *pList;
    size_t startIdx;

    inline ConstIterateReverseFromWrapper(const small_list<T, internal_count> *pList, size_t startIdx) : pList(pList), startIdx(startIdx) {}

    inline small_list_const_reverse_iterator<T, internal_count> begin()
    {
      small_list_const_reverse_iterator<T, internal_count> it = small_list_const_reverse_iterator<T, internal_count>(pList);

      it.inInternal = startIdx < internal_count;
      it.position = startIdx;
      it.subIndex = it.inInternal ? startIdx : startIdx - internal_count;

      return it;
    };

    inline size_t end() { return 0; };
  };

  inline ConstIterateReverseFromWrapper IterateReverseFrom(const size_t idx) const { return ConstIterateReverseFromWrapper(this, idx); };

  T &operator [](const size_t index);
  const T &operator [](const size_t index) const;

  inline small_list() {};
  inline small_list(const small_list &) = delete;
  inline small_list &operator = (const small_list &) = delete;

  inline small_list(small_list &&move) :
    count(move.count),
    pExtra(move.pExtra),
    extraCapacity(move.extraCapacity)
  {
    lsMemcpy(values, move.values, lsMin(internal_count, count));

    move.pExtra = nullptr;
    move.count = 0;
    move.extraCapacity = 0;
  }

  inline small_list &operator = (small_list &&move)
  {
    list_destroy(this);

    count = move.count;
    pExtra = move.pExtra;
    extraCapacity = move.extraCapacity;
    lsMemcpy(values, move.values, lsMin(internal_count, count));

    move.pExtra = nullptr;
    move.count = 0;
    move.extraCapacity = 0;

    return *this;
  }
};

template <typename T, size_t internal_count>
lsResult list_reserve(small_list<T, internal_count> *pList, const size_t count)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr, lsR_ArgumentNull);

  if (count > pList->extraCapacity + internal_count)
  {
    const size_t newCapacity = count - internal_count; // is it bad to respect their desired size here? could be `(count) & ~(internal_count - 1)`.
    LS_ERROR_CHECK(lsRealloc(&pList->pExtra, newCapacity));
    pList->extraCapacity = newCapacity;
  }

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_add(small_list<T, internal_count> *pList, const T *pItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || pItem == nullptr, lsR_ArgumentNull);

  if (pList->count < internal_count)
  {
    new (&pList->values[pList->count]) T(*pItem);
    pList->count++;
  }
  else
  {
    const size_t extraIndex = pList->count - internal_count;

    // Grow if we need to grow.
    if (pList->extraCapacity <= extraIndex)
    {
      const size_t newCapacity = (pList->extraCapacity * 2 + internal_count) & ~(internal_count - 1);
      LS_ERROR_CHECK(lsRealloc(&pList->pExtra, newCapacity));
      pList->extraCapacity = newCapacity;
    }

    new (&pList->pExtra[extraIndex]) T(*pItem);
    pList->count++;
  }

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_add(small_list<T, internal_count> *pList, T &&item)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr, lsR_ArgumentNull);

  if (pList->count < internal_count)
  {
    new (&pList->values[pList->count])T(std::move(item));
    pList->count++;
  }
  else
  {
    const size_t extraIndex = pList->count - internal_count;

    // Grow if we need to grow.
    if (pList->extraCapacity <= extraIndex)
    {
      const size_t newCapacity = (pList->extraCapacity * 2 + internal_count) & ~(internal_count - 1);
      LS_ERROR_CHECK(lsRealloc(&pList->pExtra, newCapacity));
      pList->extraCapacity = newCapacity;
    }

    new (&pList->pExtra[extraIndex])T(std::move(item));
    pList->count++;
  }

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_add(small_list<T, internal_count> *pList, const T &item)
{
  return list_add(pList, &item);
}

template <typename T, size_t internal_count, typename container>
  requires (random_iterable_sized_container_type<container, T>)
lsResult list_add_range(small_list<T, internal_count> *pList, const container &items)
{
  lsResult result = lsR_Success;

  lsAssert(pList);

  // Grow if we need to grow.
  if (pList->count + items.count > internal_count + pList->extraCapacity)
  {
    const size_t requiredCapacity = pList->count + items.count - internal_count;
    const size_t newCapacity = lsMax(pList->extraCapacity * 2 + internal_count, requiredCapacity + internal_count) & ~(internal_count - 1);
    LS_ERROR_CHECK(lsRealloc(&pList->pExtra, newCapacity));
    pList->extraCapacity = newCapacity;
  }

  {
    size_t remainingCount = items.count;
    size_t offset = 0;;

    if (pList->count < internal_count)
    {
      const size_t internalFitCount = lsMin(internal_count - pList->count, items.count);

      for (size_t i = 0; i < internalFitCount; i++)
        pList->values[pList->count + i] = items[i];

      pList->count += internalFitCount;
      offset += internalFitCount;
      remainingCount -= internalFitCount;
    }

    if (remainingCount > 0)
    {
      for (size_t i = 0; i < remainingCount; i++)
        pList->pExtra[pList->count + i] = items[offset + i];

      pList->count += remainingCount;
    }
  }

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_add_range(small_list<T, internal_count> *pList, const T *pItems, const size_t count)
{
  lsResult result = lsR_Success;

  size_t remainingCount = count;
  const T *pRemainingItems = pItems;

  lsAssert(pList != nullptr && pItems != nullptr);
  LS_ERROR_IF(pItems == nullptr, lsR_ArgumentNull);

  // Grow if we need to grow.
  if (pList->count + count > internal_count + pList->extraCapacity)
  {
    const size_t requiredCapacity = pList->count + count - internal_count;
    const size_t newCapacity = lsMax(pList->extraCapacity * 2 + internal_count, requiredCapacity + internal_count) & ~(internal_count - 1);
    LS_ERROR_CHECK(lsRealloc(&pList->pExtra, newCapacity));
    pList->extraCapacity = newCapacity;
  }

  if (pList->count < internal_count)
  {
    const size_t internalFitCount = lsMin(internal_count - pList->count, count);
    lsMemcpy(pList->values + pList->count, pItems, internalFitCount);

    pList->count += internalFitCount;
    pRemainingItems += internalFitCount;
    remainingCount -= internalFitCount;
  }

  if (remainingCount > 0)
  {
    lsMemcpy(pList->pExtra + pList->count - internal_count, pRemainingItems, remainingCount);
    pList->count += remainingCount;
  }

epilogue:
  return result;
}

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count>
inline lsResult list_insert(small_list<T, internal_count> &l, const size_t index, const T &item)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(l.count < index, lsR_ArgumentOutOfBounds);

  if (l.count == index)
    return list_add(&l, item);

  if (l.count >= internal_count + l.extraCapacity)
  {
    const size_t newCapacity = (l.extraCapacity * 2 + internal_count) & ~(internal_count - 1);
    LS_ERROR_CHECK(lsRealloc(&l.pExtra, newCapacity));
    l.extraCapacity = newCapacity;
  }

  if (l.count >= internal_count)
  {
    if (index >= internal_count)
    {
      const size_t extraValuesToMove = l.count - index;
      const size_t inExtraIndex = index - internal_count;
      T *pSrc = l.pExtra + inExtraIndex;
      lsMemmove(pSrc + 1, pSrc, extraValuesToMove);
      l.pExtra[inExtraIndex] = item;
      l.count++;
      goto epilogue;
    }
    else
    {
      const size_t extraValuesToMove = l.count - internal_count;
      lsMemmove(l.pExtra + 1, l.pExtra, extraValuesToMove);
      lsMemmove(l.pExtra, &l.values[internal_count - 1], 1);
    }
  }

  {
    const size_t internalValuesToMove = lsMin(internal_count, l.count) - index - (l.count >= internal_count ? 1 : 0);
    lsMemmove(&l.values[index + 1], &l.values[index], internalValuesToMove);
    l.values[index] = item;
    l.count++;
  }

epilogue:
  return result;
}

template <typename T, size_t internal_count>
inline lsResult list_insert(small_list<T, internal_count> &l, const size_t index, T &&item)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(l.count < index, lsR_ArgumentOutOfBounds);

  if (l.count == index)
    return list_add(&l, item);

  if (l.count >= internal_count + l.extraCapacity)
  {
    const size_t newCapacity = (l.extraCapacity * 2 + internal_count) & ~(internal_count - 1);
    LS_ERROR_CHECK(lsRealloc(&l.pExtra, newCapacity));
    l.extraCapacity = newCapacity;
  }

  if (l.count >= internal_count)
  {
    if (index >= internal_count)
    {
      const size_t extraValuesToMove = l.count - index;
      const size_t inExtraIndex = index - internal_count;
      T *pSrc = l.pExtra + inExtraIndex;
      lsMemmove(pSrc + 1, pSrc, extraValuesToMove);
      new (&l.pExtra[inExtraIndex]) T(std::move(item));
      l.count++;
      goto epilogue;
    }
    else
    {
      const size_t extraValuesToMove = l.count - internal_count;
      lsMemmove(l.pExtra + 1, l.pExtra, extraValuesToMove);
      lsMemmove(l.pExtra, &l.values[internal_count - 1], 1);
    }
  }

  {
    const size_t internalValuesToMove = lsMin(internal_count, l.count) - index - (l.count >= internal_count ? 1 : 0);
    lsMemmove(&l.values[index + 1], &l.values[index], internalValuesToMove);
    new (&l.values[index]) T(std::move(item));
    l.count++;
  }

epilogue:
  return result;
}

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count>
const T *list_get(const small_list<T, internal_count> *pList, const size_t index)
{
  lsAssert(index < pList->count);

  if (index < internal_count)
    return &pList->values[index];
  else
    return &pList->pExtra[index - internal_count];
}

template <typename T, size_t internal_count>
T *list_get(small_list<T, internal_count> *pList, const size_t index)
{
  lsAssert(index < pList->count);

  if (index < internal_count)
    return &pList->values[index];
  else
    return &pList->pExtra[index - internal_count];
}

template <typename T, size_t internal_count>
lsResult list_get_safe(const small_list<T, internal_count> *pList, const size_t index, _Out_ T *pItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || pItem == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(index >= pList->count, lsR_ArgumentOutOfBounds);

  if (index < internal_count)
    *pItem = pList->values[index];
  else
    *pItem = pList->pExtra[index - internal_count];

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_get_safe(small_list<T, internal_count> *pList, const size_t index, _Out_ T **ppItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || ppItem == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(index >= pList->count, lsR_ArgumentOutOfBounds);

  if (index < internal_count)
    *ppItem = &pList->values[index];
  else
    *ppItem = &pList->pExtra[index - internal_count];

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_get_safe(const small_list<T, internal_count> *pList, const size_t index, _Out_ T *const *ppItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || ppItem == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(index >= pList->count, lsR_ArgumentOutOfBounds);

  if (index < internal_count)
    *ppItem = &pList->values[index];
  else
    *ppItem = &pList->pExtra[index - internal_count];

epilogue:
  return result;
}

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count>
lsResult list_pop_back_safe(small_list<T, internal_count> *pList, _Out_ T *pItem = nullptr)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(pList->count == 0, lsR_ArgumentOutOfBounds);

  if (pItem != nullptr)
  {
    if (pList->count <= internal_count)
      *pItem = std::move(pList->values[pList->count - 1]);
    else
      *pItem = std::move(pList->pExtra[pList->count - 1 - internal_count]);
  }

  pList->count--;

epilogue:
  return result;
}

template <typename T, size_t internal_count>
T list_pop_back(small_list<T, internal_count> &l)
{
  lsAssert(l.count > 0);

  l.count--;

  if (l.count <= internal_count)
    return std::move(l.values[l.count]);
  else
    return std::move(l.pExtra[l.count - internal_count]);
}

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count>
lsResult list_resize(small_list<T, internal_count> *pList, const size_t newSize, const T *pDefaultItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || pDefaultItem == nullptr, lsR_ArgumentNull);

  if (newSize > pList->count)
  {
    if (newSize > pList->extraCapacity + internal_count)
      LS_ERROR_CHECK(list_reserve(pList, newSize));

    size_t i = pList->count;

    for (; i < internal_count && i < newSize; i++)
      pList->values[i] = *pDefaultItem;

    if (i < newSize)
    {
      i -= internal_count;
      const size_t extraTargetIndex = newSize - internal_count;

      for (; i < extraTargetIndex; i++)
        pList->pExtra[i] = *pDefaultItem;
    }
  }

  pList->count = newSize;

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_resize(small_list<T, internal_count> *pList, const size_t newSize, const T &defaultItem)
{
  return list_resize(pList, newSize, &defaultItem);
}

template <typename T, size_t internal_count_src, size_t internal_count_dst>
lsResult list_move(small_list<T, internal_count_src> *pSrc, const size_t srcIdx, const size_t count, small_list<T, internal_count_dst> *pDst, const size_t dstIdx)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pSrc == nullptr || pDst == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(pSrc->count <= srcIdx || pDst->count < dstIdx, lsR_ArgumentOutOfBounds);

  if (count == 0 || (pSrc == pDst && srcIdx == dstIdx))
    goto epilogue;

  if (pDst->extraCapacity + internal_count_dst < dstIdx + count)
    LS_ERROR_CHECK(list_reserve(pDst, dstIdx + count));

  if (pDst->count < dstIdx + count)
    pDst->count = dstIdx + count;

  {
    size_t srcPos = srcIdx;
    size_t dstPos = dstIdx;
    size_t countRemaining = count;

    // copy till end internal count.
    if (srcPos < internal_count_src)
    {
      const size_t srcCountInnerCopy = lsMin(internal_count_src - srcPos, countRemaining);

      if (dstPos < internal_count_dst)
      {
        const size_t dstCountInnerCopy = lsMin(internal_count_dst - dstPos, countRemaining);

        if (dstCountInnerCopy < srcCountInnerCopy)
        {
          lsMemmove(pDst->values + dstPos, pSrc->values + srcPos, dstCountInnerCopy);

          countRemaining -= dstCountInnerCopy;
          srcPos += dstCountInnerCopy;
          dstPos += dstCountInnerCopy;

          // Copy remaining internal for src.
          const size_t copyCount = srcCountInnerCopy - dstCountInnerCopy;

          lsMemmove(pDst->values + dstPos, pSrc->pExtra, copyCount);

          countRemaining -= copyCount;
          srcPos += copyCount;
          dstPos += copyCount;
        }
        else
        {
          lsMemmove(pDst->values + dstPos, pSrc->values + srcPos, srcCountInnerCopy);

          countRemaining -= srcCountInnerCopy;
          srcPos += srcCountInnerCopy;
          dstPos += srcCountInnerCopy;

          const size_t copyCount = dstCountInnerCopy - srcCountInnerCopy;

          // Copy remaining internal for dst (if any).
          if (copyCount)
          {
            lsMemmove(pDst->pExtra, pSrc->values + srcPos, copyCount);

            countRemaining -= copyCount;
            srcPos += copyCount;
            dstPos += copyCount;
          }
        }
      }
      else
      {
        lsMemmove(pDst->pExtra + dstPos - internal_count_dst, pSrc->values + srcPos, srcCountInnerCopy);

        countRemaining -= srcCountInnerCopy;
        srcPos += srcCountInnerCopy;
        dstPos += srcCountInnerCopy;
      }
    }
    else if (dstPos < internal_count_dst)
    {
      const size_t copyCount = lsMin(internal_count_dst - dstPos, countRemaining);

      lsMemmove(pDst->values + dstPos, pSrc->pExtra + srcPos - internal_count_src, copyCount);

      countRemaining -= copyCount;
      srcPos += copyCount;
      dstPos += copyCount;
    }

    // copy till end
    if (countRemaining)
      lsMemmove(pDst->pExtra + dstPos - internal_count_dst, pSrc->pExtra + srcPos - internal_count_src, countRemaining);
  }

epilogue:
  return result;
}

template <typename T, size_t internal_count>
void list_clear(small_list<T, internal_count> *pList)
{
  if (pList == nullptr)
    return;

  for (size_t i = 0; i < pList->count && i < internal_count; i++)
    pList->values[i].~T();

  if (pList->count > internal_count)
    for (size_t i = 0; i < pList->count - internal_count; i++)
      pList->pExtra[i].~T();

  pList->count = 0;
}

template <typename T, size_t internal_count>
void list_destroy(small_list<T, internal_count> *pList)
{
  if (pList == nullptr)
    return;

  for (size_t i = 0; i < pList->count && i < internal_count; i++)
    pList->values[i].~T();

  if (pList->count > internal_count)
    for (size_t i = 0; i < pList->count - internal_count; i++)
      pList->pExtra[i].~T();

  lsFreePtr(&pList->pExtra);

  pList->count = 0;
  pList->extraCapacity = 0;
}

template <typename T, size_t internal_count, typename U>
T *list_contains(small_list<T, internal_count> &list, const U &cmp)
{
  for (auto &_i : list)
    if (_i == cmp)
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count, typename U>
const T *list_contains(const small_list<T, internal_count> &list, const U &cmp)
{
  for (auto &_i : list)
    if (_i == cmp)
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count, typename U>
T *list_contains(small_list<T, internal_count> *pList, const U &cmp)
{
  lsAssert(pList != nullptr);
  return list_contains(*pList, cmp);
}

template <typename T, size_t internal_count, typename U>
const T *list_contains(const small_list<T, internal_count> *pList, const U &cmp)
{
  lsAssert(pList != nullptr);
  return list_contains(*pList, cmp);
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t internal_count>
inline small_list<T, internal_count>::~small_list()
{
  list_destroy(this);
}

template<typename T, size_t internal_count>
inline T &small_list<T, internal_count>::operator[](const size_t index)
{
  return *list_get(this, index);
}

template<typename T, size_t internal_count>
inline const T &small_list<T, internal_count>::operator[](const size_t index) const
{
  return *list_get(this, index);
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t internal_count, typename TLessFunc = std::less<T>, typename TGreaterFunc = std::greater<T>>
inline void list_sort(small_list<T, internal_count> &l)
{
  struct _internal
  {
    static void dualPivotQuickSort_partition(small_list<T, internal_count> &l, const int64_t low, const int64_t high, int64_t *pRightPivot, int64_t *pLeftPivot)
    {
      TLessFunc _less = TLessFunc();
      TGreaterFunc _greater = TGreaterFunc();

      if (_greater(l[low], l[high]))
        std::swap(l[low], l[high]);

      int64_t j = low + 1;
      int64_t g = high - 1;
      int64_t k = low + 1;

      T *pP = &l[low];
      T *pQ = &l[high];

      while (k <= g)
      {
        if (_less(l[k], *pP))
        {
          std::swap(l[k], l[j]);
          j++;
        }

        else if (!_less(l[k], *pQ))
        {
          while (_greater(l[g], *pQ) && k < g)
            g--;

          std::swap(l[k], l[g]);
          g--;

          if (_less(l[k], *pP))
          {
            std::swap(l[k], l[j]);
            j++;
          }
        }

        k++;
      }

      j--;
      g++;

      std::swap(l[low], l[j]);
      std::swap(l[high], l[g]);

      *pLeftPivot = j;
      *pRightPivot = g;
    }

    static void quickSort(small_list<T, internal_count> &l, const int64_t start, const int64_t end)
    {
      if (start < end)
      {
        int64_t leftPivot, rightPivot;

        dualPivotQuickSort_partition(l, start, end, &rightPivot, &leftPivot);

        quickSort(l, start, leftPivot - 1);
        quickSort(l, leftPivot + 1, rightPivot - 1);
        quickSort(l, rightPivot + 1, end);
      }
    }
  };

  if (l.count)
    _internal::quickSort(l, 0, l.count - 1);
}

template<typename T, size_t internal_count, typename TLessFunc = std::less<T>, typename TGreaterFunc = std::greater<T>>
inline void list_sort_descending(small_list<T, internal_count> &l)
{
  return list_sort<T, internal_count, TGreaterFunc, TLessFunc>(l);
}

template<typename T, size_t internal_count, typename TComparable>
inline void list_sort(small_list<T, internal_count> &l, const std::function<TComparable(const T &value)> &toComparable)
{
  struct _internal
  {
    static void dualPivotQuickSort_partition(small_list<T, internal_count> &l, const int64_t low, const int64_t high, int64_t *pRightPivot, int64_t *pLeftPivot, const std::function<TComparable(const T &value)> &toComparable)
    {
      if (toComparable(l[low]) > toComparable(l[high]))
        std::swap(l[low], l[high]);

      int64_t j = low + 1;
      int64_t g = high - 1;
      int64_t k = low + 1;

      T *pP = &l[low];
      T *pQ = &l[high];

      while (k <= g)
      {
        if (toComparable(l[k]) < toComparable(*pP))
        {
          std::swap(l[k], l[j]);
          j++;
        }

        else if (toComparable(l[k]) >= toComparable(*pQ))
        {
          while (toComparable(l[g]) > toComparable(*pQ) && k < g)
            g--;

          std::swap(l[k], l[g]);
          g--;

          if (toComparable(l[k]) < toComparable(*pP))
          {
            std::swap(l[k], l[j]);
            j++;
          }
        }

        k++;
      }

      j--;
      g++;

      std::swap(l[low], l[j]);
      std::swap(l[high], l[g]);

      *pLeftPivot = j;
      *pRightPivot = g;
    }

    static void quickSort(small_list<T, internal_count> &l, const int64_t start, const int64_t end, const std::function<TComparable(const T &value)> &toComparable)
    {
      if (start < end)
      {
        int64_t leftPivot, rightPivot;

        dualPivotQuickSort_partition(l, start, end, &rightPivot, &leftPivot, toComparable);

        quickSort(l, start, leftPivot - 1, toComparable);
        quickSort(l, leftPivot + 1, rightPivot - 1, toComparable);
        quickSort(l, rightPivot + 1, end, toComparable);
      }
    }
  };

  if (l.count)
    _internal::quickSort(l, 0, l.count - 1, toComparable);
}

template <typename T, size_t internal_count, typename TComparable, typename TGreater = std::greater<TComparable>>
inline void list_to_heap_percolate_recursive(small_list<T, internal_count> &l, const size_t idx, const size_t count, const std::function<TComparable(const T &value)> &toComparable)
{
  TGreater _greater;

  size_t largest = idx;
  TComparable largestVal = toComparable(l[idx]);
  const size_t lChild = idx * 2 + 1;
  const size_t rChild = idx * 2 + 2;

  if (lChild < count)
  {
    const TComparable lVal = toComparable(l[lChild]);

    if (_greater(lVal, largestVal))
    {
      largest = lChild;
      largestVal = lVal;
    }
  }

  if (rChild < count)
  {
    const TComparable rVal = toComparable(l[rChild]);

    if (_greater(rVal, largestVal))
    {
      largest = rChild;
      largestVal = rVal;
    }
  }

  if (largest != idx)
  {
    std::swap(l[idx], l[largest]);
    list_to_heap_percolate_recursive<T, internal_count, TComparable, TGreater>(l, largest, count, toComparable);
  }
}

template <typename T, size_t internal_count, typename TComparable, typename TGreater = std::greater<TComparable>>
inline void list_to_heap(small_list<T, internal_count> &l, const std::function<TComparable(const T &value)> &toComparable, const size_t count)
{
  for (int64_t i = count / 2 - 1; i >= 0; i--)
    list_to_heap_percolate_recursive<T, internal_count, TComparable, TGreater>(l, i, count, toComparable);
}

template <typename T, size_t internal_count, typename TComparable, typename TGreater = std::greater<TComparable>>
inline void list_to_heap(small_list<T, internal_count> &l, const std::function<TComparable(const T &value)> &toComparable)
{
  list_to_heap<T, TComparable, TGreater>(l, toComparable, l.count);
}

//////////////////////////////////////////////////////////////////////////

// Return index of first element for which l[i] < cmp is false or endIdx + 1 if for none.
// See: https://en.cppreference.com/w/cpp/algorithm/lower_bound
template <typename T, size_t internal_count, typename TCompare, typename TLessFunc = std::less<T>>
inline size_t sorted_list_find_lower_bound(small_list<T, internal_count> &l, const TCompare &cmp, const size_t startIdx = 0, const size_t endIdx = (size_t)-1)
{
  TLessFunc less;

  size_t first = startIdx;
  const size_t last = endIdx == (size_t)-1 ? l.count : (endIdx + 1);
  int64_t count = (int64_t)(last - first);

  while (count > 0)
  {
    const size_t step = count / 2;
    const size_t i = first + step;

    if (less(l[i], cmp))
    {
      first = i + 1;
      count -= step + 1;
    }
    else
    {
      count = step;
    }
  }

  return first;
}

template <typename T, size_t internal_count, typename TCompare, typename TGreaterFunc = std::greater<T>>
inline size_t descending_sorted_list_find_lower_bound(small_list<T, internal_count> &l, const TCompare &cmp, const size_t startIdx = 0, const size_t endIdx = (size_t)-1)
{
  return sorted_list_find_lower_bound<T, internal_count, TCompare, TGreaterFunc>(l, cmp, startIdx, endIdx);
}

// Return index of first element for which cmp < l[i] is true or endIdx + 1 if for none.
// See: https://en.cppreference.com/w/cpp/algorithm/upper_bound
template <typename T, size_t internal_count, typename TCompare, typename TLessFunc = std::less<T>>
inline size_t sorted_list_find_upper_bound(small_list<T, internal_count> &l, const TCompare &cmp, const size_t startIdx = 0, const size_t endIdx = (size_t)-1)
{
  TLessFunc less;

  size_t first = startIdx;
  const size_t last = endIdx == (size_t)-1 ? l.count : (endIdx + 1);
  int64_t count = (int64_t)(last - first);

  while (count > 0)
  {
    const size_t step = count / 2;
    const size_t it = first += step;

    if (!less(cmp, l[it]))
    {
      first = it + 1;
      count -= step + 1;
    }
    else
    {
      count = step;
    }
  }

  return first;
}

template <typename T, size_t internal_count, typename TCompare, typename TGreaterFunc = std::greater<T>>
inline size_t descending_sorted_list_find_upper_bound(small_list<T, internal_count> &l, const TCompare &cmp, const size_t startIdx = 0, const size_t endIdx = (size_t)-1)
{
  return sorted_list_find_upper_bound<T, internal_count, TCompare, TGreaterFunc>(l, cmp, startIdx, endIdx);
}

template <typename T, size_t internal_count, typename TCompare, typename TLessFunc = std::less<T>, typename TEqualsFunc = std::equal_to<T>>
inline size_t sorted_list_find(small_list<T, internal_count> &l, const TCompare &cmp, const size_t startIdx = 0, const size_t endIdx = (size_t)-1)
{
  const size_t idx = sorted_list_find_lower_bound<T, internal_count, TCompare, TLessFunc>(l, cmp, startIdx, endIdx);

  if (idx == l.count)
    return (size_t)-1;

  TEqualsFunc equals;

  if (equals(l[idx], cmp))
    return idx;

  return (size_t)-1;
}

template <typename T, size_t internal_count, typename TCompare, typename TGreaterFunc = std::greater<T>, typename TEqualsFunc = std::equal_to<T>>
inline size_t descending_sorted_list_find(small_list<T, internal_count> &l, const TCompare &cmp, const size_t startIdx = 0, const size_t endIdx = (size_t)-1)
{
  return sorted_list_find<T, internal_count, TCompare, TGreaterFunc, TEqualsFunc>(l, cmp, startIdx, endIdx);
}

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count, typename TCompare, typename TLessFunc = std::less<T>, typename TEqualsFunc = std::equal_to<T>>
inline bool sorted_list_remove_element(small_list<T, internal_count> &l, const TCompare &cmp, const size_t startIdx = 0, const size_t endIdx = (size_t)-1, _Out_opt_ T *pRemoved = nullptr)
{
  const size_t idx = sorted_list_find_lower_bound<T, internal_count, TCompare, TLessFunc>(l, cmp, startIdx, endIdx);

  if (idx == l.count)
    return false;

  TEqualsFunc equals;

  if (equals(l[idx], cmp))
  {
    if (pRemoved != nullptr)
      *pRemoved = std::move(list_remove_at(l, idx));
    else
      list_remove_at(l, idx);

    return true;
  }

  return false;
}

template <typename T, size_t internal_count, typename TCompare, typename TGreaterFunc = std::greater<T>, typename TEqualsFunc = std::equal_to<T>>
inline bool descending_sorted_list_remove_element(small_list<T, internal_count> &l, const TCompare &cmp, const size_t startIdx = 0, const size_t endIdx = (size_t)-1, _Out_opt_ T *pRemoved = nullptr)
{
  return sorted_list_remove_element<T, internal_count, TCompare, TGreaterFunc, TEqualsFunc>(l, cmp, startIdx, endIdx, pRemoved);
}

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count, typename TLessFunc = std::less<T>>
inline bool list_is_sorted(const small_list<T, internal_count> &l)
{
  if (l.count == 0)
    return true;

  auto it = l.begin();
  auto end = l.end();
  TLessFunc less;

  const auto *pVal = &(*it);
  ++it;

  while (it != end)
  {
    const T &v = *it;

    if (!less(*pVal, v))
      return false;

    pVal = &v;
    ++it;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////

//template <typename T, size_t internal_count, typename TIndex, bool OutputData>
//  requires (std::is_integral_v<TIndex>)
//void _list_remove_sorted_indexes_internal(small_list<T, internal_count> &l, const TIndex *pIndexes, const size_t count, _Out_opt_ T *pOut = nullptr)
//{
//  lsAssert(l.count >= count);
//  lsAssert(pIndexes != nullptr);
//
//  for (size_t i = 0; i < count; i++)
//  {
//    const size_t oldIdx = pIndexes[i];
//    const size_t newIdx = oldIdx - i;
//    lsAssert(oldIdx >= i);
//    lsAssert(oldIdx - i < l.count - i);
//
//    if constexpr (!OutputData)
//      l.pValues[oldIdx].~T();
//    else
//      new (&pOut[i]) T(std::move(l.pValues[oldIdx]));
//
//    size_t moveCount;
//
//    if (i + 1 == count)
//    {
//      moveCount = l.count - oldIdx - 1;
//    }
//    else
//    {
//      lsAssert(pIndexes[i + 1] >= i + 1);
//      lsAssert(pIndexes[i + 1] - i - 1 >= newIdx);
//
//      moveCount = pIndexes[i + 1] - i - 1 - newIdx;
//    }
//
//    if constexpr (std::is_trivially_move_constructible_v<T>)
//    {
//      lsMemmove(l.pValues + newIdx, l.pValues + oldIdx + 1, moveCount);
//    }
//    else
//    {
//      for (int64_t j = moveCount - 1; j >= 0; j--)
//        new (l.pValues + newIdx + j) T(std::move(l.pValues[oldIdx + 1 + j]));
//    }
//  }
//
//  l.count -= count;
//}
//
//template <typename T, size_t internal_count, typename TIndex>
//  requires (std::is_integral_v<TIndex>)
//void list_remove_sorted_indexes(small_list<T, internal_count> &l, const TIndex *pIndexes, const size_t count, _Out_opt_ T *pOut = nullptr)
//{
//  if (pOut == nullptr)
//    _list_remove_sorted_indexes_internal<T, internal_count, TIndex, false>(l, pIndexes, count, pOut);
//  else
//    _list_remove_sorted_indexes_internal<T, internal_count, TIndex, true>(l, pIndexes, count, pOut);
//}
//
//template <typename T, size_t internal_count, typename TIndex, typename TPtr, bool Move>
//  requires (std::is_integral_v<TIndex>)
//lsResult _list_insert_sorted_indexes_internal(small_list<T, internal_count> &l, const TIndex *pIndexes, const size_t count, TPtr pValues)
//{
//  lsResult result = lsR_Success;
//
//  lsAssert(pIndexes != nullptr || count == 0);
//  lsAssert(pValues != nullptr || count == 0);
//
//  if (l.count + count > l.capacity)
//    LS_ERROR_CHECK(list_reserve(&l, l.count + count));
//
//  if (count > 0)
//  {
//    const size_t nextOldIdx = pIndexes[count - 1];
//    const size_t moveCount = l.count - nextOldIdx;
//
//    if constexpr (std::is_trivially_move_constructible_v<T>)
//    {
//      lsMemmove(l.pValues + nextOldIdx + count, l.pValues + nextOldIdx, moveCount);
//    }
//    else
//    {
//      for (int64_t j = moveCount - 1; j >= 0; j--)
//        new (l.pValues + nextOldIdx + count + j) T(std::move(l.pValues[nextOldIdx + j]));
//    }
//  }
//
//  for (int64_t i = count - 1; i >= 0; i--)
//  {
//    const size_t oldIdx = pIndexes[i];
//    const size_t newIdx = oldIdx + i;
//
//    if constexpr (Move)
//      new (l.pValues + newIdx) T(std::move(pValues[i]));
//    else
//      l.pValues[newIdx] = pValues[i];
//
//    if (i > 0)
//    {
//      const size_t nextOldtIdx = pIndexes[i - 1];
//      lsAssert(nextOldtIdx <= oldIdx);
//      const size_t moveCount = oldIdx - nextOldtIdx;
//
//      if constexpr (std::is_trivially_move_constructible_v<T>)
//      {
//        lsMemmove(l.pValues + newIdx - moveCount, l.pValues + newIdx - moveCount - i, moveCount);
//      }
//      else
//      {
//        for (size_t j = 0; j < moveCount; j++)
//          new (l.pValues + newIdx - moveCount) T(std::move(l.pValues[newIdx - moveCount - i + j]));
//      }
//    }
//  }
//
//  l.count += count;
//
//epilogue:
//  return result;
//}
//
//template <typename T, size_t internal_count, typename TIndex>
//  requires (std::is_integral_v<TIndex>)
//lsResult list_insert_sorted_indexes_move(small_list<T, internal_count> &l, const TIndex *pIndexes, const size_t count, T *pValues)
//{
//  return _list_insert_sorted_indexes_internal<T, internal_count, TIndex, T *, true>(l, pIndexes, count, pValues);
//}
//
//template <typename T, size_t internal_count, typename TIndex>
//  requires (std::is_integral_v<TIndex>)
//lsResult list_insert_sorted_indexes(small_list<T, internal_count> &l, const TIndex *pIndexes, const size_t count, const T *pValues)
//{
//  return _list_insert_sorted_indexes_internal<T, internal_count, TIndex, const T *, false>(l, pIndexes, count, pValues);
//}

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count>
inline T list_remove_at(small_list<T, internal_count> &l, const size_t index)
{
  lsAssert(l.count > index);

  T ret(std::move(l[index]));

  if (index < internal_count)
  {
    const size_t minInternal = lsMin(internal_count, l.count);
    const size_t inInternalCount = minInternal - 1 - index;
    lsMemmove(&l.values[index], &l.values[index + 1], inInternalCount);

    if (l.count > internal_count)
    {
      new (&l.values[internal_count - 1]) T(std::move(l.pExtra[0]));

      const size_t inExtraCount = (l.count - internal_count - 1);
      lsMemmove(l.pExtra, l.pExtra + 1, inExtraCount);
    }
  }
  else
  {
    const size_t inExternalCount = l.count - 1 - index;
    const size_t extraIndex = index - internal_count;
    lsMemmove(&l.pExtra[extraIndex], &l.pExtra[extraIndex + 1], inExternalCount);
  }

  l.count--;

  return ret;
}

template <typename T, size_t internal_count, typename TComparable, typename TEqual = std::equal_to<TComparable>>
inline bool list_remove_element(small_list<T, internal_count> &l, const TComparable &element, T *pOut = nullptr)
{
  TEqual _equal;

  if (l.count == 0)
    return false;

  size_t index = (size_t)-1;

  for (const auto &_elem : l)
  {
    ++index;

    if constexpr (std::is_same_v<std::equal_to<TComparable>, TEqual> && !std::is_same_v<TComparable, T>) // workaround for comparing with a different non-comparable type.
    {
      if (_elem == element)
      {
        if (pOut == nullptr)
          list_remove_at(l, index);
        else
          *pOut = list_remove_at(l, index);

        return true;
      }
    }
    else
    {
      if (_equal(_elem, element))
      {
        if (pOut == nullptr)
          list_remove_at(l, index);
        else
          *pOut = list_remove_at(l, index);

        return true;
      }
    }
  }

  return false;
}

template <typename T, size_t internal_count, typename TComparable, typename TEqual = std::equal_to<TComparable>>
inline bool list_remove_all_matching_elements(small_list<T, internal_count> &l, const TComparable &element)
{
  TEqual _equal;

  if (l.count == 0)
    return false;

  size_t index = l.count;
  bool found = false;

  for (const auto &_elem : l.IterateReverse())
  {
    --index;

    if constexpr (std::is_same_v<std::equal_to<TComparable>, TEqual> && !std::is_same_v<TComparable, T>) // workaround for comparing with a different non-comparable type.
    {
      if (_elem == element)
      {
        list_remove_at(l, index);
        found = true;
      }
    }
    else
    {
      if (_equal(_elem, element))
      {
        list_remove_at(l, index);
        found = true;
      }
    }
  }

  return found;
}

//////////////////////////////////////////////////////////////////////////

// This is functionally a near-copy of `_list_remove_insert_internal`.
template <typename T, size_t internal_count>
inline void list_move(small_list<T, internal_count> &l, const size_t srcIdx, const size_t targetIdx)
{
  lsAssert(l.count > srcIdx);
  lsAssert(l.count > targetIdx);

  if (srcIdx == targetIdx)
    return;

  T value(std::move(l[srcIdx]));

  size_t moveStartIdx, moveEndIdx, moveCount;

  if (srcIdx < targetIdx)
  {
    moveStartIdx = srcIdx + 1;
    moveEndIdx = srcIdx;
    moveCount = targetIdx - srcIdx;
  }
  else
  {
    moveStartIdx = targetIdx;
    moveEndIdx = targetIdx + 1;
    moveCount = srcIdx - targetIdx;
  }

  if (srcIdx < internal_count && targetIdx < internal_count)
  {
    lsMemmove(l.values + moveEndIdx, l.values + moveStartIdx, moveCount);
    new (&l.values[targetIdx]) T(std::move(value));
  }
  else if (srcIdx >= internal_count && targetIdx >= internal_count)
  {
    const size_t moveStartExt = moveStartIdx - internal_count;
    const size_t moveEndExt = moveEndIdx - internal_count;

    lsMemmove(l.pExtra + moveEndExt, l.pExtra + moveStartExt, moveCount);
    new (&l.pExtra[targetIdx - internal_count]) T(std::move(value));
  }
  else
  {
    const size_t internalMoveCount = internal_count - lsMax(moveStartIdx, moveEndIdx);
    const size_t externalMoveCount = moveCount - internalMoveCount - 1;

    if (srcIdx > targetIdx)
    {
      lsMemmove(l.pExtra + 1, l.pExtra, externalMoveCount);
      new (&l.pExtra[0]) T(std::move(l.values[internal_count - 1]));
      lsMemmove(l.values + moveEndIdx, l.values + moveStartIdx, internalMoveCount);
      new (&l.values[targetIdx]) T(std::move(value));
    }
    else
    {
      lsMemmove(l.values + moveEndIdx, l.values + moveStartIdx, internalMoveCount);
      new (&l.values[internal_count - 1]) T(std::move(l.pExtra[0]));
      lsMemmove(l.pExtra, l.pExtra + 1, externalMoveCount);
      new (&l.pExtra[targetIdx - internal_count]) T(std::move(value));
    }
  }
}

template <typename T, size_t internal_count, typename ValueType, bool MoveInsert>
  requires (std::is_same_v<std::remove_cvref_t<ValueType>, T>)
inline T _list_remove_insert_internal(small_list<T, internal_count> &l, const size_t removeIndex, ValueType value, const size_t insertIndex)
{
  lsAssert(l.count > removeIndex);
  lsAssert(l.count > insertIndex);

  T *pVal = &l[removeIndex];
  T ret(std::move(*pVal));

  if (removeIndex == insertIndex)
  {
    if constexpr (MoveInsert)
      new (pVal) T(std::move(value));
    else
      *pVal = value;

    return ret;
  }

  size_t moveStartIdx, moveEndIdx, moveCount;

  if (removeIndex < insertIndex)
  {
    moveStartIdx = removeIndex + 1;
    moveEndIdx = removeIndex;
    moveCount = insertIndex - removeIndex;
  }
  else
  {
    moveStartIdx = insertIndex;
    moveEndIdx = insertIndex + 1;
    moveCount = removeIndex - insertIndex;
  }

  if (removeIndex < internal_count && insertIndex < internal_count)
  {
    lsMemmove(l.values + moveEndIdx, l.values + moveStartIdx, moveCount);

    if constexpr (MoveInsert)
      new (&l.values[insertIndex]) T(std::move(value));
    else
      l.values[insertIndex] = value;
  }
  else if (removeIndex >= internal_count && insertIndex >= internal_count)
  {
    const size_t moveStartExt = moveStartIdx - internal_count;
    const size_t moveEndExt = moveEndIdx - internal_count;

    lsMemmove(l.pExtra + moveEndExt, l.pExtra + moveStartExt, moveCount);

    if constexpr (MoveInsert)
      new (&l.pExtra[insertIndex - internal_count]) T(std::move(value));
    else
      l.pExtra[insertIndex - internal_count] = value;
  }
  else
  {
    const size_t internalMoveCount = internal_count - lsMax(moveStartIdx, moveEndIdx);
    const size_t externalMoveCount = moveCount - internalMoveCount - 1;

    if (removeIndex > insertIndex)
    {
      lsMemmove(l.pExtra + 1, l.pExtra, externalMoveCount);
      new (&l.pExtra[0]) T(std::move(l.values[internal_count - 1]));
      lsMemmove(l.values + moveEndIdx, l.values + moveStartIdx, internalMoveCount);

      if constexpr (MoveInsert)
        new (&l.values[insertIndex]) T(std::move(value));
      else
        l.values[insertIndex] = value;
    }
    else
    {
      lsMemmove(l.values + moveEndIdx, l.values + moveStartIdx, internalMoveCount);
      new (&l.values[internal_count - 1]) T(std::move(l.pExtra[0]));
      lsMemmove(l.pExtra, l.pExtra + 1, externalMoveCount);

      if constexpr (MoveInsert)
        new (&l.pExtra[insertIndex - internal_count]) T(std::move(value));
      else
        l.pExtra[insertIndex - internal_count] = value;
    }
  }

  return ret;
}

template <typename T, size_t internal_count>
inline T list_remove_insert(small_list<T, internal_count> &l, const size_t removeIndex, const T &value, const size_t insertIndex)
{
  return _list_remove_insert_internal<T, internal_count, const T &, false>(l, removeIndex, value, insertIndex);
}

template <typename T, size_t internal_count>
inline T list_remove_insert(small_list<T, internal_count> &l, const size_t removeIndex, T &&value, const size_t insertIndex)
{
  return _list_remove_insert_internal<T, internal_count, T &&, true>(l, removeIndex, std::forward<T>(value), insertIndex);
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t internal_count>
inline small_list_iterator<T, internal_count>::small_list_iterator(small_list<T, internal_count> *pList) :
  pList(pList)
{ }

template<typename T, size_t internal_count>
inline T &small_list_iterator<T, internal_count>::operator*()
{
  if (inInternal)
    return pList->values[subIndex];
  else
    return pList->pExtra[subIndex];
}

template<typename T, size_t internal_count>
inline const T &small_list_iterator<T, internal_count>::operator*() const
{
  if (inInternal)
    return pList->values[subIndex];
  else
    return pList->pExtra[subIndex];
}

template<typename T, size_t internal_count>
inline bool small_list_iterator<T, internal_count>::operator!=(const size_t maxCount) const
{
  return position < maxCount;
}

template<typename T, size_t internal_count>
inline bool small_list_iterator<T, internal_count>::operator!=(const small_list_iterator<T, internal_count> &it) const
{
  return position < it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_iterator<T, internal_count>::operator!=(const small_list_reverse_iterator<T, internal_count> &it) const
{
  return (int64_t)position < it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_iterator<T, internal_count>::operator!=(const small_list_const_iterator<T, internal_count> &it) const
{
  return position < it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_iterator<T, internal_count>::operator!=(const small_list_const_reverse_iterator<T, internal_count> &it) const
{
  return (int64_t)position < it.position;
}

template<typename T, size_t internal_count>
inline small_list_iterator<T, internal_count> &small_list_iterator<T, internal_count>::operator++()
{
  position++;
  subIndex++;

  if (inInternal && subIndex == internal_count)
  {
    inInternal = false;
    subIndex = 0;
  }

  return *this;
}

template<typename T, size_t internal_count>
inline small_list_reverse_iterator<T, internal_count>::small_list_reverse_iterator(small_list<T, internal_count> *pList) :
  pList(pList),
  position((size_t)lsMax((int64_t)0, (int64_t)pList->count - 1))
{
  inInternal = position < internal_count;
  subIndex = inInternal ? position : position - internal_count;
}

template<typename T, size_t internal_count>
inline T &small_list_reverse_iterator<T, internal_count>::operator*()
{
  if (inInternal)
    return pList->values[subIndex];
  else
    return pList->pExtra[subIndex];
}

template<typename T, size_t internal_count>
inline const T &small_list_reverse_iterator<T, internal_count>::operator*() const
{
  if (inInternal)
    return pList->values[subIndex];
  else
    return pList->pExtra[subIndex];
}

template<typename T, size_t internal_count>
inline bool small_list_reverse_iterator<T, internal_count>::operator!=(const size_t startIndex) const
{
  return position >= (int64_t)startIndex;
}

template<typename T, size_t internal_count>
inline bool small_list_reverse_iterator<T, internal_count>::operator!=(const small_list_iterator<T, internal_count> &it) const
{
  return position >= (int64_t)it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_reverse_iterator<T, internal_count>::operator!=(const small_list_reverse_iterator<T, internal_count> &it) const
{
  return position >= it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_reverse_iterator<T, internal_count>::operator!=(const small_list_const_iterator<T, internal_count> &it) const
{
  return position >= (int64_t)it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_reverse_iterator<T, internal_count>::operator!=(const small_list_const_reverse_iterator<T, internal_count> &it) const
{
  return position >= it.position;
}

template<typename T, size_t internal_count>
inline small_list_reverse_iterator<T, internal_count> &small_list_reverse_iterator<T, internal_count>::operator++()
{
  position--;
  subIndex--;

  if (subIndex < 0)
  {
    inInternal = true;
    subIndex = internal_count - 1;
  }

  return *this;
}

template<typename T, size_t internal_count>
inline small_list_const_iterator<T, internal_count>::small_list_const_iterator(const small_list<T, internal_count> *pList) :
  pList(pList)
{ }

template<typename T, size_t internal_count>
inline const T &small_list_const_iterator<T, internal_count>::operator*() const
{
  if (inInternal)
    return pList->values[subIndex];
  else
    return pList->pExtra[subIndex];
}

template<typename T, size_t internal_count>
inline bool small_list_const_iterator<T, internal_count>::operator!=(const size_t maxCount) const
{
  return position < maxCount;
}

template<typename T, size_t internal_count>
inline bool small_list_const_iterator<T, internal_count>::operator!=(const small_list_iterator<T, internal_count> &it) const
{
  return position < it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_const_iterator<T, internal_count>::operator!=(const small_list_reverse_iterator<T, internal_count> &it) const
{
  return (int64_t)position < it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_const_iterator<T, internal_count>::operator!=(const small_list_const_iterator<T, internal_count> &it) const
{
  return position < it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_const_iterator<T, internal_count>::operator!=(const small_list_const_reverse_iterator<T, internal_count> &it) const
{
  return (int64_t)position < it.position;
}

template<typename T, size_t internal_count>
inline small_list_const_iterator<T, internal_count> &small_list_const_iterator<T, internal_count>::operator++()
{
  position++;
  subIndex++;

  if (inInternal && subIndex == internal_count)
  {
    inInternal = false;
    subIndex = 0;
  }

  return *this;
}

template<typename T, size_t internal_count>
inline small_list_const_reverse_iterator<T, internal_count>::small_list_const_reverse_iterator(const small_list<T, internal_count> *pList) :
  pList(pList),
  position((size_t)lsMax((int64_t)0, (int64_t)pList->count - 1))
{
  inInternal = position < internal_count;
  subIndex = inInternal ? position : position - internal_count;
}

template<typename T, size_t internal_count>
inline const T &small_list_const_reverse_iterator<T, internal_count>::operator*() const
{
  if (inInternal)
    return pList->values[subIndex];
  else
    return pList->pExtra[subIndex];
}

template<typename T, size_t internal_count>
inline bool small_list_const_reverse_iterator<T, internal_count>::operator!=(const size_t startIndex) const
{
  return position >= (int64_t)startIndex;
}

template<typename T, size_t internal_count>
inline bool small_list_const_reverse_iterator<T, internal_count>::operator!=(const small_list_iterator<T, internal_count> &it) const
{
  return position >= (int64_t)it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_const_reverse_iterator<T, internal_count>::operator!=(const small_list_reverse_iterator<T, internal_count> &it) const
{
  return position >= it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_const_reverse_iterator<T, internal_count>::operator!=(const small_list_const_iterator<T, internal_count> &it) const
{
  return position >= (int64_t)it.position;
}

template<typename T, size_t internal_count>
inline bool small_list_const_reverse_iterator<T, internal_count>::operator!=(const small_list_const_reverse_iterator<T, internal_count> &it) const
{
  return position >= it.position;
}

template<typename T, size_t internal_count>
inline small_list_const_reverse_iterator<T, internal_count> &small_list_const_reverse_iterator<T, internal_count>::operator++()
{
  position--;
  subIndex--;

  if (subIndex < 0)
  {
    inInternal = true;
    subIndex = internal_count - 1;
  }

  return *this;
}
