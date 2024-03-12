#pragma once

template <typename T, size_t TSize>
struct local_list
{
  T data[TSize];
  size_t size = 0;

  T &operator[](const size_t index)
  {
    return data[index];
  }

  const T &operator[](const size_t index) const
  {
    return data[index];
  }
};

template <typename T, size_t TSize>
bool local_list_add(local_list<T, TSize> &list, const T value)
{
  if (list.size >= TSize)
    return false;

  list.data[list.size] = value;
  list.size++;

  return true;
}

template <typename T, size_t TSize>
bool local_list_remove(local_list<T, TSize> &list, const size_t index)
{
  if (index >= list.size)
    return false;

  if (index == list.size - 1)
  {
    list.size--;
    return true;
  }

  list.size--;

  for (size_t i = index; i < list.size; i++)
    list.data[i] = list.data[i + 1];

  return true;
}

template <typename T, size_t TSize>
void local_list_clear(local_list<T, TSize> &list)
{
  list.size = 0;
}
