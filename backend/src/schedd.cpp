#include "schedd.h"

#include <mutex>
#include <time.h>

std::atomic<size_t> _UserDataEpoch = 0;
std::atomic<size_t> _EventDataEpoch = 0;
std::atomic<size_t> _ExplicitlyRequestsRescheduleEpoch = 0;

pool<user> _Users;
pool<event> _Events;

static std::mutex _ThreadLock;
static pool<user_id_info> _SessionIdToUserId;

//////////////////////////////////////////////////////////////////////////

struct sortable_event
{
  size_t event_id;
  uint64_t score;

  inline sortable_event() {}
  inline sortable_event(const size_t event_id, const uint64_t score) : event_id(event_id), score(score) {}

  inline bool operator < (const sortable_event &other) const
  {
    return (score < other.score);
  }

  inline bool operator > (const sortable_event &other) const
  {
    return (score > other.score);
  }
};

// coc where does one put this?
#ifndef _WIN32
#fail not implemented
#endif

struct time_info
{
  size_t dayIndex;
  time_point_t time;
};

uint64_t get_score_for_event(const event evnt);
time_info get_current_day_and_time();

//////////////////////////////////////////////////////////////////////////

lsResult reschedule_events_for_user(const size_t userId) // Assumes mutex lock
{
  lsResult result = lsR_Success;

  small_list<sortable_event, 128> userEvents;
  const time_info time = get_current_day_and_time();
  weekday_flags today = (weekday_flags)(1 << time.dayIndex);

  for (const auto &&_evnt : _Events)
  {
    // Is Event executable on current weekday?
    if (_evnt.pItem->possibleExecutionDays & today)
    {
      // Is event due today?
      if (_evnt.pItem->lastCompletedTime + _evnt.pItem->repetitionTimeSpan <= time.time || _evnt.pItem->lastCompletedTime == 0)
      {
        // Is User Participating in Event?
        bool foundUserId = false;
        for (const auto &uId : _evnt.pItem->userIds)
        {
          if (userId == uId)
          {
            foundUserId = true;
            break;
          }
        }

        if (foundUserId)
        {
          const auto score = get_score_for_event(*_evnt.pItem);
          LS_DEBUG_ERROR_ASSERT(small_list_add(&userEvents, sortable_event(_evnt.index, score)));
        }
      }
    }
  }

  small_list_sort_descending(userEvents);

  // Pick.
  time_span_t freeTime = 0; // needs to be initialized before `LS_ERROR_IF`.
  
  user *pUser = pool_get(&_Users, userId);
  LS_ERROR_IF(pUser == nullptr, lsR_ResourceNotFound);

  local_list_clear(&pUser->tasksForCurrentDay);

  freeTime = pUser->availableTimePerDay[time.dayIndex];

  if (freeTime >= 0 && pUser->tasksForCurrentDay.capacity() > 0)
  {
    for (const auto &_sortable_event : userEvents)
    {
      event *pEvent = pool_get(&_Events, _sortable_event.event_id);
      lsAssert(pEvent != nullptr);

      if (pEvent->durationTimeSpan > freeTime)
        continue;

      LS_DEBUG_ERROR_ASSERT(local_list_add(&pUser->tasksForCurrentDay, _sortable_event.event_id));
      freeTime -= pEvent->durationTimeSpan;
     
      lsAssert(freeTime >= 0);

      if (freeTime <= 0 || pUser->tasksForCurrentDay.count == pUser->tasksForCurrentDay.capacity())
        break;
    }
  }

  // TODO: Error handling.
  // Should it be an error if we couldn't schedule any task?
epilogue:
  return result;
}

uint64_t get_score_for_event(const event evnt)
{
  // pretend it won't be executed today...

  const time_info currentTime = get_current_day_and_time();

  // calculate time span between today and the last time it should've been completed
  time_span_t diffTodayTarget;
  if (evnt.lastCompletedTime == 0)
    diffTodayTarget = currentTime.time - evnt.creationTime; // this is not completely accurate as the day of creation prbably hasn't been the first due day
  else
    diffTodayTarget = currentTime.time - (evnt.lastCompletedTime + evnt.repetitionTimeSpan);

  // How many days unitl the next possible execution day after today?

  // TODO: use bitscan reverse
  weekday_flags today = (weekday_flags)(1 << currentTime.dayIndex); // TODO: simplify stuff as we now know the dayIndex!
  size_t countUntilNextPossibleDay = 0;
  
  if (!(evnt.possibleExecutionDays & ~today)) // if today is the only possible execution day
  {
    countUntilNextPossibleDay = DaysPerWeek;
  }
  else if (evnt.possibleExecutionDays & wF_All)
  {
    countUntilNextPossibleDay = 1;
  }
  else
  {
    bool stoppedAtSunday = false;

    lsAssert(evnt.possibleExecutionDays != wF_None);
    for (size_t i = 0; i < DaysPerWeek; i++)
    {
      if (today << i & wF_Sunday)
      {
        stoppedAtSunday = true;
        countUntilNextPossibleDay = i;
        break;
      }

      if (evnt.possibleExecutionDays & (today << (i + 1)))
      {
        countUntilNextPossibleDay = i + 1;
        break;
      }
    }

    if (stoppedAtSunday)
    {
      for (size_t i = 0; i < DaysPerWeek; i++)
      {
        if (evnt.possibleExecutionDays & (wF_Monday << i))
        {
          countUntilNextPossibleDay += i + 1;
          break;
        }
      }
    }
  }

  // How many repetition time spans fit into the time period from last and next possible execution (after today)?
  size_t dueTime = days_from_time_span(diffTodayTarget) + countUntilNextPossibleDay;
  size_t repetitionInDays = days_from_time_span(evnt.repetitionTimeSpan);
  uint64_t dueTimePeriodCount = dueTime % repetitionInDays;
  dueTimePeriodCount += 100 * ((dueTime - dueTimePeriodCount * repetitionInDays) / repetitionInDays);

  return evnt.weight + dueTimePeriodCount * evnt.weightGrowthFactor;
}

//////////////////////////////////////////////////////////////////////////

lsResult assign_session_token(const char *username, _Out_ uint32_t *pOutSessionId)
{
  lsResult result = lsR_Success;

  // Scope Lock.
  {
    std::scoped_lock lock(_ThreadLock);

    size_t userId = 0; // initialzing for comipler warning
    bool userNotFound = true;

    // iterate pool checking for username
    for (const auto &&_user : _Users)
    {
      if (strncmp(_user.pItem->username, username, LS_ARRAYSIZE(_user.pItem->username)) == 0) // if name matches.
      {
        userId = _user.index;
        userNotFound = false;
        break;
      }
    }

    LS_ERROR_IF(userNotFound, lsR_InvalidParameter);

    // Assign Session Id.
    {
      *pOutSessionId = (uint32_t)lsGetRand(); // collisions should be pretty rare... hopefully.

      // Add User Info to List
      user_id_info userInfo;
      userInfo.sessionId = *pOutSessionId;

      lsAssert(userId <= lsMaxValue<uint32_t>());
      userInfo.userId = (uint32_t)userId;

      size_t _unused;
      LS_ERROR_CHECK(pool_add(&_SessionIdToUserId, userInfo, &_unused));
    }
  }

epilogue:
  return result;
}

lsResult invalidate_session_token(const uint32_t sessionId)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_id : _SessionIdToUserId)
      if (_id.pItem->sessionId == sessionId)
        LS_ERROR_CHECK(pool_remove_safe(&_SessionIdToUserId, _id.index));
  }

epilogue:
  return result;
}

lsResult add_new_user(const user usr)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    size_t _unused;
    LS_DEBUG_ERROR_ASSERT(pool_add(&_Users, usr, &_unused));
  }

  _UserDataEpoch++;

  return result;
}

lsResult get_user_info(const size_t userId, _Out_ user_info *pOutInfo)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    user *pUser = pool_get(&_Users, userId);
    LS_ERROR_IF(pUser == nullptr, lsR_ResourceNotFound);

    pOutInfo->id = userId;
    strncpy(pOutInfo->name, pUser->username, LS_ARRAYSIZE(pOutInfo->name));
  }

epilogue:
  return result;
}

lsResult get_user_id_from_session_id(const uint32_t sessionId, _Out_ size_t *pUserId)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_item : _SessionIdToUserId)
    {
      if (_item.pItem->sessionId == sessionId)
      {
        *pUserId = _item.pItem->userId;
        goto epilogue;
      }
    }

    LS_ERROR_SET(lsR_InvalidParameter);
  }

epilogue:
  return result;
}

lsResult get_available_time(const size_t userId, _Out_ local_list<time_span_t, DaysPerWeek> *pOutAvailableTime)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    user *pUser = pool_get(&_Users, userId);
    LS_ERROR_IF(pUser == nullptr, lsR_ResourceNotFound);

    *pOutAvailableTime = pUser->availableTimePerDay;
  }

epilogue:
  return result;
}

lsResult replace_available_time(const size_t userId, const local_list<time_span_t, DaysPerWeek> availableTime)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    user *pUser = pool_get(&_Users, userId);
    LS_ERROR_IF(pUser == nullptr, lsR_ResourceNotFound);

    pUser->availableTimePerDay = availableTime;
  }

  _UserDataEpoch++;

epilogue:
  return result;
}

lsResult add_new_event(event evnt)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    size_t _unused;
    LS_ERROR_CHECK(pool_add(&_Events, evnt, &_unused));
  }

  _EventDataEpoch++;

epilogue:
  return result;
}

lsResult get_current_events_from_user_id(const size_t userId, _Out_ local_list<event_info, MaxEventsPerUserPerDay> *pOutCurrentEvents)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    const user *pUser = pool_get(&_Users, userId);
    LS_ERROR_IF(pUser == nullptr, lsR_ResourceNotFound);

    for (const size_t eventId : pUser->tasksForCurrentDay)
    {
      event *pEvent = pool_get(&_Events, eventId);
      LS_ERROR_IF(pEvent == nullptr, lsR_ResourceNotFound);

      event_info info;
      info.id = eventId;
      strncpy(info.name, pEvent->name, LS_ARRAYSIZE(info.name));
      info.durationInMinutes = minutes_from_time_span(pEvent->durationTimeSpan);

      LS_ERROR_CHECK(local_list_add(pOutCurrentEvents, info));
    }
  }

epilogue:
  return result;
}

lsResult get_completed_events_for_current_day(const size_t userId, _Out_ local_list<event_info, MaxEventsPerUserPerDay> *pOutCompletedTasks)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    user *pUser = pool_get(&_Users, userId);
    LS_ERROR_IF(pUser == nullptr, lsR_ResourceNotFound);

    for (const size_t eventId : pUser->completedTasksForCurrentDay)
    {
      event *pEvent = pool_get(&_Events, eventId);
      LS_ERROR_IF(pEvent == nullptr, lsR_ResourceNotFound);

      event_info info;
      info.id = eventId;
      info.durationInMinutes = minutes_from_time_span(pEvent->durationTimeSpan);
      strncpy(info.name, pEvent->name, LS_ARRAYSIZE(info.name));

      LS_ERROR_CHECK(local_list_add(pOutCompletedTasks, info));
    }
  }

epilogue:
  return result;
}

lsResult update_task(const size_t id, event evnt) // evnt is intentionally non-const as it's creation time etc. will be updated to the old values.
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    event *pStoredEvent = nullptr;
    LS_ERROR_CHECK(pool_get_safe(&_Events, id, &pStoredEvent));

    evnt.creationTime = pStoredEvent->creationTime;
    evnt.lastCompletedTime = pStoredEvent->lastCompletedTime;
    evnt.lastModifiedTime = get_current_time();

    *pStoredEvent = evnt;
  }

  _EventDataEpoch++;

epilogue:
  return result;
}

lsResult set_event_last_completed_time(const size_t eventId, const time_point_t time)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    event *pEvent = nullptr;
    LS_ERROR_CHECK(pool_get_safe(&_Events, eventId, &pEvent));

    pEvent->lastCompletedTime = time;
  }

  _EventDataEpoch++;

epilogue:
  return result;
}

lsResult add_completed_task(const size_t eventId, const size_t userId)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    user *pUser = nullptr;
    LS_ERROR_CHECK(pool_get_safe(&_Users, userId, &pUser));
    LS_ERROR_CHECK(local_list_add(&pUser->completedTasksForCurrentDay, eventId));
  }

epilogue:
  return result;
}

lsResult search_events_by_name(const char *searchTerm, _Out_ local_list<event_info, MaxSearchResults> *pOutSearchResults)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_evnt : _Events)
    {
      if (strstr(_evnt.pItem->name, searchTerm) != nullptr)
      {
        event_info info;
        info.id = _evnt.index;
        strncpy(info.name, _evnt.pItem->name, LS_ARRAYSIZE(info.name));
        info.durationInMinutes = minutes_from_time_span(_evnt.pItem->durationTimeSpan);

        LS_DEBUG_ERROR_ASSERT(local_list_add(pOutSearchResults, info));

        if (pOutSearchResults->count == pOutSearchResults->capacity())
          break;
      }
    }
  }

  goto epilogue;
epilogue:
  return result;
}

lsResult search_events_by_user_by_name(const size_t userId, const char *searchTerm, _Out_ local_list<event_info, MaxSearchResults> *pOutSearchResults)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_evnt : _Events)
    {
      bool userIdFound = false;

      for (const auto &_id : _evnt.pItem->userIds)
      {
        if (_id == userId)
        {
          userIdFound = true;
          break;
        }
      }

      if (strstr(_evnt.pItem->name, searchTerm) != nullptr)
      {
        event_info info;
        info.id = _evnt.index;
        strncpy(info.name, _evnt.pItem->name, LS_ARRAYSIZE(info.name));
        info.durationInMinutes = minutes_from_time_span(_evnt.pItem->durationTimeSpan);

        LS_DEBUG_ERROR_ASSERT(local_list_add(pOutSearchResults, info));

        if (pOutSearchResults->count == pOutSearchResults->capacity())
          break;
      }
    }
  }

  goto epilogue;
epilogue:
  return result;
}

lsResult search_users_by_name(const char *searchTerm, _Out_ local_list<user_info, MaxSearchResults> *pOutSearchResults)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_user : _Users)
    {
      if (strstr(_user.pItem->username, searchTerm))
      {
        user_info info;
        info.id = _user.index;
        strncpy(info.name, _user.pItem->username, LS_ARRAYSIZE(info.name));

        LS_DEBUG_ERROR_ASSERT(local_list_add(pOutSearchResults, info));

        if (pOutSearchResults->count == pOutSearchResults->capacity())
          break;
      }
    }
  }

  goto epilogue;
epilogue:
  return result;
}

lsResult get_all_event_ids_for_user(const size_t userId, _Out_ local_list<size_t, MaxSearchResults> *pOutEventIds)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_evnt : _Events)
    {
      for (const auto &_id : _evnt.pItem->userIds)
      {
        if (_id != userId)
          continue;

        LS_DEBUG_ERROR_ASSERT(local_list_add(pOutEventIds, _evnt.index));

        if (pOutEventIds->count == pOutEventIds->capacity())
          goto epilogue;
      }
    }
  }

epilogue:
  return result;
}

lsResult get_event(const size_t id, _Out_ event *pEvent)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    const event *pStoredEvent = pool_get(&_Events, id);
    LS_ERROR_IF(pStoredEvent == nullptr, lsR_ResourceNotFound);

    *pEvent = *pStoredEvent;
  }

epilogue:
  return result;
}

bool user_name_exists(const char *username)
{
  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_user : _Users)
      if ((strncmp(_user.pItem->username, username, LS_ARRAYSIZE(_user.pItem->username)) == 0))
        return false;
  }
  return true;
}

time_point_t get_current_time()
{
  return (time_point_t)time(nullptr);
}

size_t get_days_since_new_year()
{
  time_t t = time(nullptr);
  return localtime(&t)->tm_yday;
}

size_t get_hours_since_midnight()
{
  time_t t = time(nullptr);
  return localtime(&t)->tm_hour;
}

time_info get_current_day_and_time()
{
  time_t t = time(nullptr);
  size_t day = localtime(&t)->tm_wday; // Days since Sunday

  time_info ret;
  ret.dayIndex = day == 0 ? 6 : day - 1;
  ret.time = time_point_t(t);

  return ret;
}

time_span_t time_span_from_days(const size_t days)
{
  constexpr int64_t day_to_unix_timestamp = 60 * 60 * 24;

  return (time_span_t)(day_to_unix_timestamp * days);
}

size_t days_from_time_span(const time_span_t timeSpan)
{
  constexpr int64_t day_to_unix_timestamp = 60 * 60 * 24;

  return (size_t)(timeSpan / day_to_unix_timestamp);
}

time_span_t time_span_from_minutes(const size_t minutes)
{
  constexpr int64_t minute_to_unix_timestamp = 60;

  return (time_span_t)(minute_to_unix_timestamp * minutes);
}

size_t minutes_from_time_span(const time_span_t timeSpan)
{
  constexpr int64_t minute_to_unix_timestamp = 60;

  return (size_t)(timeSpan / minute_to_unix_timestamp);
}
