#pragma once

#include "core.h"

template <typename T, size_t internal_count>
struct local_list;

template <typename T, size_t internal_count>
struct local_list_reverse_iterator;

template <typename T, size_t internal_count>
struct local_list_const_iterator;

template <typename T, size_t internal_count>
struct local_list_const_reverse_iterator;

template <typename T, size_t internal_count>
struct local_list_iterator
{
  local_list<T, internal_count> *pList = nullptr;
  size_t position = 0;

  local_list_iterator(local_list<T, internal_count> *pList);
  T &operator *();
  const T &operator *() const;
  bool operator != (const size_t maxCount) const;
  bool operator != (const local_list_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_reverse_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_const_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_const_reverse_iterator<T, internal_count> &it) const;
  local_list_iterator<T, internal_count> &operator++();
};

template <typename T, size_t internal_count>
struct local_list_reverse_iterator
{
  local_list<T, internal_count> *pList = nullptr;
  int64_t position = 0;

  local_list_reverse_iterator(local_list<T, internal_count> *pList);
  T &operator *();
  const T &operator *() const;
  bool operator != (const size_t startIndex) const;
  bool operator != (const local_list_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_reverse_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_const_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_const_reverse_iterator<T, internal_count> &it) const;
  local_list_reverse_iterator<T, internal_count> &operator++();
};

template <typename T, size_t internal_count>
struct local_list_const_iterator
{
  const local_list<T, internal_count> *pList = nullptr;
  size_t position = 0;

  local_list_const_iterator(const local_list<T, internal_count> *pList);
  const T &operator *() const;
  bool operator != (const size_t maxCount) const;
  bool operator != (const local_list_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_reverse_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_const_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_const_reverse_iterator<T, internal_count> &it) const;
  local_list_const_iterator<T, internal_count> &operator++();
};

template <typename T, size_t internal_count>
struct local_list_const_reverse_iterator
{
  const local_list<T, internal_count> *pList = nullptr;
  int64_t position = 0;

  local_list_const_reverse_iterator(const local_list<T, internal_count> *pList);
  const T &operator *() const;
  bool operator != (const size_t startIndex) const;
  bool operator != (const local_list_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_reverse_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_const_iterator<T, internal_count> &it) const;
  bool operator != (const local_list_const_reverse_iterator<T, internal_count> &it) const;
  local_list_const_reverse_iterator<T, internal_count> &operator++();
};

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count>
struct _local_list_reverse_it_wrapper
{
  local_list<T, internal_count> *pList;
  inline local_list_reverse_iterator<T, internal_count> begin() { return local_list_reverse_iterator<T, internal_count>(pList); };
  inline size_t end() { return 0; };
};

template <typename T, size_t internal_count>
struct _local_list_it_from_wrapper
{
  local_list<T, internal_count> *pList;
  size_t startIdx;

  inline local_list_iterator<T, internal_count> begin()
  {
    const local_list_iterator<T, internal_count> it = local_list_iterator<T, internal_count>(pList);
    it.position = startIdx;

    return it;
  };

  inline size_t end() { return pList->count; };
};

template <typename T, size_t internal_count>
struct _local_list_const_it_from_wrapper
{
  const local_list<T, internal_count> *pList;
  size_t startIdx;

  inline local_list_const_iterator<T, internal_count> begin()
  {
    const local_list_const_iterator<T, internal_count> it = local_list_const_iterator<T, internal_count>(pList);
    it.position = startIdx;

    return it;
  };

  inline size_t end() { return pList->count; };
};

template <typename T, size_t internal_count>
struct _local_list_it_reverse_from_wrapper
{
  local_list<T, internal_count> *pList;
  size_t startIdx;

  inline local_list_reverse_iterator<T, internal_count> begin()
  {
    const local_list_reverse_iterator<T, internal_count> it = local_list_reverse_iterator<T, internal_count>(pList);
    it.position = startIdx;

    return it;
  };

  inline size_t end() { return 0; };
};

template <typename T, size_t internal_count>
struct _local_list_const_it_reverse_from_wrapper
{
  local_list<T, internal_count> *pList;
  size_t startIdx;

  inline local_list_const_reverse_iterator<T, internal_count> begin()
  {
    const local_list_const_reverse_iterator<T, internal_count> it = local_list_const_reverse_iterator<T, internal_count>(pList);
    it.position = startIdx;

    return it;
  };

  inline size_t end() { return 0; };
};

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t internal_count>
struct local_list
{
  T values[internal_count];
  size_t count = 0;

  inline local_list_iterator<T, internal_count> begin() { return local_list_iterator<T, internal_count>(this); };
  inline local_list_const_iterator<T, internal_count> begin() const { return local_list_const_iterator<T, internal_count>(this); };
  inline size_t end() const { return count; };
  inline constexpr size_t capacity() const { return internal_count; };

  ~local_list();

  inline _local_list_reverse_it_wrapper<T, internal_count> IterateReverse() { return { this }; };
  inline _local_list_it_from_wrapper<T, internal_count> IterateFrom(const size_t idx) { return { this, idx }; };
  inline _local_list_const_it_from_wrapper<T, internal_count> IterateFrom(const size_t idx) const { return { this, idx }; };
  inline _local_list_it_reverse_from_wrapper<T, internal_count> IterateReverseFrom(const size_t idx) { return { this, idx }; };
  inline _local_list_const_it_reverse_from_wrapper<T, internal_count> IterateReverseFrom(const size_t idx) const { return { this, idx }; };

  T &operator [](const size_t index);
  const T &operator [](const size_t index) const;

  inline local_list() {};
  inline local_list(const local_list &) = default;
  inline local_list &operator = (const local_list &) = default;

  inline local_list(local_list &&move) :
    count(move.count)
  {
    lsMemcpy(values, move.values, lsMin(internal_count, count));

    move.count = 0;
  }

  inline local_list &operator = (local_list &&move)
  {
    list_destroy(this);

    count = move.count;
    lsMemcpy(values, move.values, lsMin(internal_count, count));

    move.count = 0;

    return *this;
  }
};

template <typename T, size_t internal_count>
lsResult list_add(local_list<T, internal_count> *pList, const T *pItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || pItem == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(pList->count >= internal_count, lsR_ResourceFull);

  pList->values[pList->count] = *pItem;
  pList->count++;

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_add(local_list<T, internal_count> *pList, T &&item)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(pList->count >= internal_count, lsR_ResourceFull);

  new (&pList->values[pList->count])T(std::move(item));
  pList->count++;

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_add(local_list<T, internal_count> *pList, const T &item)
{
  return list_add(pList, &item);
}

template <typename T, size_t internal_count>
lsResult list_add_range(local_list<T, internal_count> *pList, const T *pItems, const size_t count)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || pItems == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(pList->count + count >= internal_count, lsR_ResourceFull);

  lsMemcpy(pList->values + pList->count, pItems, count);

  pList->count += count;

epilogue:
  return result;
}

template <typename T, size_t internal_count>
const T *list_get(const local_list<T, internal_count> *pList, const size_t index)
{
  lsAssert(index < pList->count);

  return &pList->values[index];
}

template <typename T, size_t internal_count>
T *list_get(local_list<T, internal_count> *pList, const size_t index)
{
  lsAssert(index < pList->count);

  return &pList->values[index];
}

template <typename T, size_t internal_count>
lsResult list_get_safe(const local_list<T, internal_count> *pList, const size_t index, _Out_ T *pItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || pItem == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(index >= pList->count, lsR_ArgumentOutOfBounds);

  *pItem = pList->values[index];

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_get_safe(local_list<T, internal_count> *pList, const size_t index, _Out_ T **ppItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || ppItem == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(index >= pList->count, lsR_ArgumentOutOfBounds);

  *ppItem = &pList->values[index];

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_get_safe(const local_list<T, internal_count> *pList, const size_t index, _Out_ T *const *ppItem)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr || ppItem == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(index >= pList->count, lsR_ArgumentOutOfBounds);

  *ppItem = &pList->values[index];

epilogue:
  return result;
}

template <typename T, size_t internal_count>
lsResult list_pop_back_safe(local_list<T, internal_count> *pList, _Out_ T *pItem = nullptr)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(pList->count == 0, lsR_ArgumentOutOfBounds);

  if (pItem != nullptr)
    *pItem = std::move(pList->values[pList->count - 1]);

  pList->count--;

epilogue:
  return result;
}

template <typename T, size_t internal_count>
T list_pop_back(local_list<T, internal_count> &list)
{
  lsAssert(list.count > 0);

  list.count--;
  return std::move(list.values[list.count]);
}

template <typename T, size_t internal_count>
lsResult list_pop_front_safe(local_list<T, internal_count> *pList, _Out_ T *pItem = nullptr)
{
  lsResult result = lsR_Success;

  LS_ERROR_IF(pList == nullptr, lsR_ArgumentNull);
  LS_ERROR_IF(pList->count == 0, lsR_ArgumentOutOfBounds);

  if (pItem != nullptr)
    *pItem = std::move(pList->values[0]);

  pList->count--;

  if constexpr (std::is_trivially_move_constructible_v<T>)
  {
    lsMemmove(pList->values, pList->values + 1, pList->count);
  }
  else
  {
    for (size_t i = 0; i < pList->count; i++)
      new (&pList->values[i]) T(std::move(pList->values[i + 1]));
  }

epilogue:
  return result;
}

template <typename T, size_t internal_count>
T list_pop_front(local_list<T, internal_count> &list)
{
  lsAssert(list.count > 0);

  T tmp = std::move(list.values[0]);
  list.count--;

  if constexpr (std::is_trivially_move_constructible_v<T>)
  {
    lsMemmove(list.values, list.values + 1, list.count);
  }
  else
  {
    for (size_t i = 0; i < list.count; i++)
      new (&list.values[i]) T(std::move(list.values[i + 1]));
  }

  return tmp;
}

template <typename T, size_t internal_count>
void list_clear(local_list<T, internal_count> *pList)
{
  if (pList == nullptr)
    return;

  for (size_t i = 0; i < pList->count; i++)
    pList->values[i].~T();

  pList->count = 0;
}

template <typename T, size_t internal_count>
void list_destroy(local_list<T, internal_count> *pList)
{
  if (pList == nullptr)
    return;

  for (size_t i = 0; i < pList->count; i++)
    pList->values[i].~T();

  pList->count = 0;
}

template <typename T, size_t internal_count, typename U>
T *list_contains(local_list<T, internal_count> &list, const U &cmp)
{
  for (auto &_i : list)
    if (_i == cmp)
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count, typename U>
const T *list_contains(const local_list<T, internal_count> &list, const U &cmp)
{
  for (auto &_i : list)
    if (_i == cmp)
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count, typename U>
const T *list_contains(const local_list<T, internal_count> &list, const U &cmp, bool (*CmpFunc)(T, U))
{
  for (auto &_i : list)
    if (CmpFunc(_i, cmp))
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count, typename U>
const T *list_contains(const local_list<T, internal_count> &list, const U &cmp, bool (*CmpFunc)(U, T))
{
  for (auto &_i : list)
    if (CmpFunc(cmp, _i))
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count>
const T *list_contains(const local_list<T, internal_count> &list, const T &cmp, int (*ComparatorResultFunc)(T, T))
{
  for (auto &_i : list)
    if (0 == ComparatorResultFunc(_i, cmp))
      return &_i;

  return nullptr;
}

template <typename T, size_t internal_count, typename U>
T *list_contains(local_list<T, internal_count> *pList, const U &cmp)
{
  lsAssert(pList != nullptr);
  return list_contains(*pList, cmp);
}

template <typename T, size_t internal_count, typename U>
const T *list_contains(const local_list<T, internal_count> *pList, const U &cmp)
{
  lsAssert(pList != nullptr);
  return list_contains(*pList, cmp);
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t internal_count>
inline local_list<T, internal_count>::~local_list()
{
  list_destroy(this);
}

template<typename T, size_t internal_count>
inline T &local_list<T, internal_count>::operator[](const size_t index)
{
  return *list_get(this, index);
}

template<typename T, size_t internal_count>
inline const T &local_list<T, internal_count>::operator[](const size_t index) const
{
  return *list_get(this, index);
}

//////////////////////////////////////////////////////////////////////////

template<typename T, size_t internal_count>
inline local_list_iterator<T, internal_count>::local_list_iterator(local_list<T, internal_count> *pList) :
  pList(pList)
{ }

template<typename T, size_t internal_count>
inline T &local_list_iterator<T, internal_count>::operator*()
{
  return pList->values[position];
}

template<typename T, size_t internal_count>
inline const T &local_list_iterator<T, internal_count>::operator*() const
{
  return pList->values[position];
}

template<typename T, size_t internal_count>
inline bool local_list_iterator<T, internal_count>::operator!=(const size_t maxCount) const
{
  return position < maxCount;
}

template<typename T, size_t internal_count>
inline bool local_list_iterator<T, internal_count>::operator!=(const local_list_iterator<T, internal_count> &it) const
{
  return position < it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_iterator<T, internal_count>::operator!=(const local_list_reverse_iterator<T, internal_count> &it) const
{
  return (int64_t)position < it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_iterator<T, internal_count>::operator!=(const local_list_const_iterator<T, internal_count> &it) const
{
  return position < it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_iterator<T, internal_count>::operator!=(const local_list_const_reverse_iterator<T, internal_count> &it) const
{
  return (int64_t)position < it.position;
}

template<typename T, size_t internal_count>
inline local_list_iterator<T, internal_count> &local_list_iterator<T, internal_count>::operator++()
{
  position++;

  return *this;
}

template<typename T, size_t internal_count>
inline local_list_reverse_iterator<T, internal_count>::local_list_reverse_iterator(local_list<T, internal_count> *pList) :
  pList(pList),
  position((size_t)lsMax((int64_t)0, (int64_t)pList->count - 1))
{ }

template<typename T, size_t internal_count>
inline T &local_list_reverse_iterator<T, internal_count>::operator*()
{
  return pList->values[position];
}

template<typename T, size_t internal_count>
inline const T &local_list_reverse_iterator<T, internal_count>::operator*() const
{
  return pList->values[position];
}

template<typename T, size_t internal_count>
inline bool local_list_reverse_iterator<T, internal_count>::operator!=(const size_t startIndex) const
{
  return position >= (int64_t)startIndex;
}

template<typename T, size_t internal_count>
inline bool local_list_reverse_iterator<T, internal_count>::operator!=(const local_list_iterator<T, internal_count> &it) const
{
  return position >= (int64_t)it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_reverse_iterator<T, internal_count>::operator!=(const local_list_reverse_iterator<T, internal_count> &it) const
{
  return position >= it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_reverse_iterator<T, internal_count>::operator!=(const local_list_const_iterator<T, internal_count> &it) const
{
  return position >= (int64_t)it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_reverse_iterator<T, internal_count>::operator!=(const local_list_const_reverse_iterator<T, internal_count> &it) const
{
  return position >= it.position;
}

template<typename T, size_t internal_count>
inline local_list_reverse_iterator<T, internal_count> &local_list_reverse_iterator<T, internal_count>::operator++()
{
  position--;

  return *this;
}

template<typename T, size_t internal_count>
inline local_list_const_iterator<T, internal_count>::local_list_const_iterator(const local_list<T, internal_count> *pList) :
  pList(pList)
{ }

template<typename T, size_t internal_count>
inline const T &local_list_const_iterator<T, internal_count>::operator*() const
{
  return pList->values[position];
}

template<typename T, size_t internal_count>
inline bool local_list_const_iterator<T, internal_count>::operator!=(const size_t maxCount) const
{
  return position < maxCount;
}

template<typename T, size_t internal_count>
inline bool local_list_const_iterator<T, internal_count>::operator!=(const local_list_iterator<T, internal_count> &it) const
{
  return position < it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_const_iterator<T, internal_count>::operator!=(const local_list_reverse_iterator<T, internal_count> &it) const
{
  return (int64_t)position < it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_const_iterator<T, internal_count>::operator!=(const local_list_const_iterator<T, internal_count> &it) const
{
  return position < it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_const_iterator<T, internal_count>::operator!=(const local_list_const_reverse_iterator<T, internal_count> &it) const
{
  return (int64_t)position < it.position;
}

template<typename T, size_t internal_count>
inline local_list_const_iterator<T, internal_count> &local_list_const_iterator<T, internal_count>::operator++()
{
  position++;

  return *this;
}

template<typename T, size_t internal_count>
inline local_list_const_reverse_iterator<T, internal_count>::local_list_const_reverse_iterator(const local_list<T, internal_count> *pList) :
  pList(pList),
  position((size_t)lsMax((int64_t)0, (int64_t)pList->count - 1))
{ }

template<typename T, size_t internal_count>
inline const T &local_list_const_reverse_iterator<T, internal_count>::operator*() const
{
  return pList->values[position];
}

template<typename T, size_t internal_count>
inline bool local_list_const_reverse_iterator<T, internal_count>::operator!=(const size_t startIndex) const
{
  return position >= (int64_t)startIndex;
}

template<typename T, size_t internal_count>
inline bool local_list_const_reverse_iterator<T, internal_count>::operator!=(const local_list_iterator<T, internal_count> &it) const
{
  return position >= (int64_t)it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_const_reverse_iterator<T, internal_count>::operator!=(const local_list_reverse_iterator<T, internal_count> &it) const
{
  return position >= it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_const_reverse_iterator<T, internal_count>::operator!=(const local_list_const_iterator<T, internal_count> &it) const
{
  return position >= (int64_t)it.position;
}

template<typename T, size_t internal_count>
inline bool local_list_const_reverse_iterator<T, internal_count>::operator!=(const local_list_const_reverse_iterator<T, internal_count> &it) const
{
  return position >= it.position;
}

template<typename T, size_t internal_count>
inline local_list_const_reverse_iterator<T, internal_count> &local_list_const_reverse_iterator<T, internal_count>::operator++()
{
  position--;

  return *this;
}
