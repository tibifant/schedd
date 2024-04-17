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
  T & operator *();
  const T & operator *() const;
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
  T & operator *();
  const T & operator *() const;
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
  const T & operator *() const;
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

  inline struct
  {
    small_list<T, internal_count> *pList;
    inline small_list_reverse_iterator<T, internal_count> begin() { return small_list_reverse_iterator<T, internal_count>(pList); };
    inline size_t end() { return 0; };
  } IterateReverse() { return { this }; };

  ~small_list();

  inline struct
  {
    small_list<T, internal_count> *pList;
    size_t startIdx;

    inline small_list_iterator<T, internal_count> begin()
    {
      const small_list_iterator<T, internal_count> it = small_list_iterator<T, internal_count>(pList);
      
      it.inInternal = startIdx < internal_count;
      it.position = startIdx;
      it.subIndex = it.inInternal ? startIdx : startIdx - internal_count;
      
      return it;
    };
    
    inline size_t end() { return count; };
  } IterateFrom(const size_t idx) { return { this, idx }; };

  inline struct
  {
    small_list<T, internal_count> *pList;
    size_t startIdx;

    inline small_list_const_iterator<T, internal_count> begin()
    {
      const small_list_const_iterator<T, internal_count> it = small_list_const_iterator<T, internal_count>(pList);
      
      it.inInternal = startIdx < internal_count;
      it.position = startIdx;
      it.subIndex = it.inInternal ? startIdx : startIdx - internal_count;
      
      return it;
    };
    
    inline size_t end() { return count; };
  } IterateFrom(const size_t idx) const { return { this, idx }; };

  inline struct
  {
    small_list<T, internal_count> *pList;
    size_t startIdx;

    inline small_list_reverse_iterator<T, internal_count> begin()
    {
      const small_list_reverse_iterator<T, internal_count> it = small_list_reverse_iterator<T, internal_count>(pList);

      it.inInternal = startIdx < internal_count;
      it.position = startIdx;
      it.subIndex = it.inInternal ? startIdx : startIdx - internal_count;

      return it;
    };

    inline size_t end() { return 0; };
  } IterateReverseFrom(const size_t idx) { return { this, idx }; };

  inline struct
  {
    small_list<T, internal_count> *pList;
    size_t startIdx;

    inline small_list_const_reverse_iterator<T, internal_count> begin()
    {
      const small_list_const_reverse_iterator<T, internal_count> it = small_list_const_reverse_iterator<T, internal_count>(pList);

      it.inInternal = startIdx < internal_count;
      it.position = startIdx;
      it.subIndex = it.inInternal ? startIdx : startIdx - internal_count;

      return it;
    };

    inline size_t end() { return 0; };
  } IterateReverseFrom(const size_t idx) const { return { this, idx }; };

  T &operator [](const size_t index);
  const T &operator [](const size_t index) const;

  inline small_list() {};
  inline small_list(const small_list &) = delete;
  inline small_list & operator = (const small_list &) = delete;
  
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
    small_list_destroy(this);
    
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
lsResult small_list_reserve(small_list<T, internal_count> *pList, const size_t count)
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
lsResult small_list_add(small_list<T, internal_count> *pList, const T *pItem)
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
lsResult small_list_add(small_list<T, internal_count> *pList, T &&item)
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
lsResult small_list_add(small_list<T, internal_count> *pList, const T &item)
{
  return small_list_add(pList, &item);
}

template <typename T, size_t internal_count>
lsResult small_list_add_range(small_list<T, internal_count> *pList, const T *pItems, const size_t count)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || pItems == nullptr, lsR_ArgumentNull);

  // Grow if we need to grow.
  if (pList->count + count > internal_count + pList->extraCapacity)
  {
    const size_t requiredCapacity = pList->count + count - internal_count;
    const size_t newCapacity = lsMax(pList->extraCapacity * 2 + internal_count, requiredCapacity + internal_count) & ~(internal_count - 1);
    LS_ERROR_CHECK(lsRealloc(&pList->pExtra, newCapacity));
    pList->extraCapacity = newCapacity;
  }

  size_t remainingCount = count;
  const T *pRemainingItems = pItems;

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

template <typename T, size_t internal_count>
const T * small_list_get(const small_list<T, internal_count> *pList, const size_t index)
{
  lsAssert(index < pList->count);

  if (index < internal_count)
    return &pList->values[index];
  else
    return &pList->pExtra[index - internal_count];
}

template <typename T, size_t internal_count>
T * small_list_get(small_list<T, internal_count> *pList, const size_t index)
{
  lsAssert(index < pList->count);

  if (index < internal_count)
    return &pList->values[index];
  else
    return &pList->pExtra[index - internal_count];
}

template <typename T, size_t internal_count>
lsResult small_list_get_safe(const small_list<T, internal_count> *pList, const size_t index, _Out_ T *pItem)
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
lsResult small_list_get_safe(small_list<T, internal_count> *pList, const size_t index, _Out_ T **ppItem)
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
lsResult small_list_get_safe(const small_list<T, internal_count> *pList, const size_t index, _Out_ T * const *ppItem)
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
lsResult small_list_pop_back_safe(small_list<T, internal_count> *pList, _Out_ T *pItem = nullptr)
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
lsResult small_list_resize(small_list<T, internal_count> *pList, const size_t newSize, const T *pDefaultItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || pDefaultItem == nullptr, lsR_ArgumentNull);

  if (newSize > pList->count)
  {
    if (newSize > pList->extraCapacity + internal_count)
      LS_ERROR_CHECK(small_list_reserve(pList, newSize));

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
lsResult small_list_resize(small_list<T, internal_count> *pList, const size_t newSize, const T &defaultItem)
{
  return small_list_resize(pList, newSize, &defaultItem);
}

template <typename T, size_t internal_count_src, size_t internal_count_dst>
lsResult small_list_move(small_list<T, internal_count_src> *pSrc, const size_t srcIdx, const size_t count, small_list<T, internal_count_dst> *pDst, const size_t dstIdx)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pSrc == nullptr || pDst == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(pSrc->count <= srcIdx || pDst->count < dstIdx, lsR_ArgumentOutOfBounds);

  if (count == 0 || (pSrc == pDst && srcIdx == dstIdx))
    goto epilogue;

  if (pDst->extraCapacity + internal_count_dst < dstIdx + count)
    LS_ERROR_CHECK(small_list_reserve(pDst, dstIdx + count));

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
void small_list_clear(small_list<T, internal_count> *pList)
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
void small_list_destroy(small_list<T, internal_count> *pList)
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
T * small_list_contains(small_list<T, internal_count> &list, const U &cmp)
{
  for (auto &_i : list)
    if (_i == cmp)
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count, typename U>
const T * small_list_contains(const small_list<T, internal_count> &list, const U &cmp)
{
  for (auto &_i : list)
    if (_i == cmp)
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count, typename U>
T * small_list_contains(small_list<T, internal_count> *pList, const U &cmp)
{
  lsAssert(pList != nullptr);
  return small_list_contains(*pList, cmp);
}

template <typename T, size_t internal_count, typename U>
const T * small_list_contains(const small_list<T, internal_count> *pList, const U &cmp)
{
  lsAssert(pList != nullptr);
  return small_list_contains(*pList, cmp);
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t internal_count>
inline small_list<T, internal_count>::~small_list()
{
  small_list_destroy(this);
}

template<typename T, size_t internal_count>
inline T &small_list<T, internal_count>::operator[](const size_t index)
{
  return *small_list_get(this, index);
}

template<typename T, size_t internal_count>
inline const T &small_list<T, internal_count>::operator[](const size_t index) const
{
  return *small_list_get(this, index);
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t internal_count, typename TLessFunc = std::less<T>, typename TGreaterFunc = std::greater<T>>
inline void small_list_sort(small_list<T, internal_count> &l)
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
inline void small_list_sort_descending(small_list<T, internal_count> &l)
{
  return small_list_sort<T, TGreaterFunc, TLessFunc>(l);
}

template<typename T, size_t internal_count, typename TComparable>
inline void small_list_sort(small_list<T, internal_count> &l, const std::function<TComparable (const T &value)> &toComparable)
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
inline void small_list_to_heap_percolate_recursive(small_list<T, internal_count> &l, const size_t idx, const size_t count, const std::function<TComparable(const T &value)> &toComparable)
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
    small_list_to_heap_percolate_recursive(l, largest, count, toComparable);
  }
}

template <typename T, size_t internal_count, typename TComparable, typename TGreater = std::greater<TComparable>>
inline void small_list_to_heap(small_list<T, internal_count> &l, const std::function<TComparable(const T &value)> &toComparable, const size_t count)
{
  for (int64_t i = count / 2 - 1; i >= 0; i--)
    small_list_to_heap_percolate_recursive(l, i, count, toComparable);
}

template <typename T, size_t internal_count, typename TComparable, typename TGreater = std::greater<TComparable>>
inline void small_list_to_heap(small_list<T, internal_count> &l, const std::function<TComparable(const T &value)> &toComparable)
{
  small_list_to_heap<T, internal_count, TComparable, TGreater>(l, toComparable, l.count);
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t internal_count>
inline small_list_iterator<T, internal_count>::small_list_iterator(small_list<T, internal_count> *pList) :
  pList(pList)
{ }

template<typename T, size_t internal_count>
inline T & small_list_iterator<T, internal_count>::operator*()
{
  if (inInternal)
    return pList->values[subIndex];
  else
    return pList->pExtra[subIndex];
}

template<typename T, size_t internal_count>
inline const T & small_list_iterator<T, internal_count>::operator*() const
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
  inInternal = position > internal_count;
  subIndex = inInternal ? position - internal_count : position;
}

template<typename T, size_t internal_count>
inline T & small_list_reverse_iterator<T, internal_count>::operator*()
{
  if (inInternal)
    return pList->values[subIndex];
  else
    return pList->pExtra[subIndex];
}

template<typename T, size_t internal_count>
inline const T & small_list_reverse_iterator<T, internal_count>::operator*() const
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
  inInternal = position > internal_count;
  subIndex = inInternal ? position - internal_count : position;
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
