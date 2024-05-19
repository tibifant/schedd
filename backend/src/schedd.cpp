#include "schedd.h"

#include <mutex>
#include <time.h>

std::atomic<size_t> _UserChangingStatus = 0;
std::atomic<size_t> _EventChangingStatus = 0;
std::atomic<size_t> _ExplicitlyRequestsReschedule = 0;

pool<user> _Users;
pool<event> _Events;

static std::mutex _ThreadLock;
static local_list<user_id_info, maxUserAmount * maxSessionsPerUser> _UserInfo; // Why do we need this an dif so, is there any need for seraialzing this? Sounds like this completely obsolet and we coulf just search for the sessionId in the _Users pool and there we would get the userId. I don't get it right now.

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

void reschedule_events_for_user(const size_t userId) // Assumes mutex lock
{
  small_list<sortable_event, 128> events;
  const time_info time = get_current_day_and_time();
  weekday_flags today = (weekday_flags)(1 << time.dayIndex);

  for (const auto &&_item : _Events)
  {
    // Is Event executable on current weekday?
    if (_item.pItem->possibleExecutionDays & today)
    {
      // Is event due today?
      if (_item.pItem->lastCompletedTime + _item.pItem->repetitionTimeSpan <= time.time || _item.pItem->lastCompletedTime == 0)
      {
        // Is User Participating in Event?
        bool foundUserId = false;
        for (const auto &uId : _item.pItem->userIds)
        {
          if (userId == uId)
          {
            foundUserId = true;
            break;
          }
        }

        if (foundUserId)
        {
          const auto score = get_score_for_event(*_item.pItem);
          small_list_add(&events, sortable_event(_item.index, score));
        }
      }
    }
  }

  small_list_sort_descending(events);

  // Pick.
  user *pUser = pool_get(&_Users, userId);
  local_list_clear(&pUser->tasksForCurrentDay);

  time_span_t availableTime = pUser->availableTimePerDay[time.dayIndex];

  for (const auto &_item : events)
  {
    if (availableTime >= 0)
    {
      event evnt = *pool_get(&_Events, _item.event_id);

      if (evnt.durationTimeSpan <= availableTime)
      {
        local_list_add(&pUser->tasksForCurrentDay, _item.event_id);
        availableTime -= evnt.durationTimeSpan;
      }
    }
    else
    {
      break;
    }
  }

  // TODO: Error handling.
  // Should it be an error if we couldn't schedule any task?
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
  // TODO: this seems sooo 'lino head overcomplicating the world'... not even sure if it works right now...
  // TODO: use bitscan reverse
  weekday_flags today = (weekday_flags)(1 << currentTime.dayIndex); // TODO simplify stuff as we now know the dayIndex!
  size_t countUntilNextPossibleDay = 0;
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

  // How many repetition time spans fit into the timeperiod from last and next possible execution (after today)?
  size_t dueTime = days_from_time_span(diffTodayTarget) + countUntilNextPossibleDay;
  size_t repetitionInDays = days_from_time_span(evnt.repetitionTimeSpan);
  uint64_t dueTimePeriodCount = dueTime % repetitionInDays;
  dueTimePeriodCount += 100 * ((dueTime - dueTimePeriodCount * repetitionInDays) / repetitionInDays);

  return evnt.weight + dueTimePeriodCount * evnt.weightGrowthFactor;
}

//////////////////////////////////////////////////////////////////////////

lsResult assign_session_token(const char *username, _Out_ int32_t *pOutSessionId)
{
  lsResult result = lsR_Success;

  // Scope Lock.
  {
    std::scoped_lock lock(_ThreadLock);

    size_t userId = 0; // If no value is assigned there's a warning, but we won't ever use this uninitilaized as we would not pass the `error_if`.
    user usr;
    bool userNotFound = true;

    // iterate pool checking for username
    for (const auto &&_item : _Users)
    {
      if (strncmp(_item.pItem->username, username, LS_ARRAYSIZE(_item.pItem->username)) == 0) // if name matches.
      {
        userId = _item.index;
        usr = *_item.pItem;
        userNotFound = false;
        break;
      }
    }

    LS_ERROR_IF(userNotFound, lsR_InvalidParameter);

    // Assign Session Id.
    {
      *pOutSessionId = (int32_t)lsGetRand();

      // Check for sessionId duplicates
      for (const auto &_item : _UserInfo)
        while (_item.sessionId == *pOutSessionId)
          *pOutSessionId = (int32_t)lsGetRand();
   
      session_token token;
      token.sessionId = *pOutSessionId;

      // if user already has maximum amount of userids, replace first
      if (usr.sessionTokens.count == maxSessionsPerUser)
      {
        int32_t firstSessionIdOfUser = usr.sessionTokens[0].sessionId;
        usr.sessionTokens[0] = token;

        for (auto &_item : _UserInfo)
          if (_item.sessionId == firstSessionIdOfUser)
            _item.sessionId = token.sessionId;
      }
      else
      {
        local_list_add(&usr.sessionTokens, token);
        
        // Add User Info to List
        user_id_info userInfo;
        userInfo.sessionId = *pOutSessionId;
        userInfo.userId = userId;
        local_list_add(&_UserInfo, userInfo); // TODO: Either cap the amount of maximum users or handle having no free slot for another userInfo!
      }
    }
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

  _UserChangingStatus++;

  return result;
}

lsResult get_user_info(const size_t userId, _Out_ user_info *pOutInfo)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    user usr = *pool_get(&_Users, userId);

    pOutInfo->id = userId;
    strncpy(pOutInfo->name, usr.username, LS_ARRAYSIZE(pOutInfo->name));
  }

  return result;
}

lsResult get_user_id_from_session_id(const int32_t sessionId, _Out_ size_t *pUserId)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    bool sessionIdNotFound = true;

    for (const auto &_item : _UserInfo)
    {
      if (_item.sessionId == sessionId)
      {
        *pUserId = _item.userId;
        sessionIdNotFound = false;
        break;
      }
    }

    LS_ERROR_IF(sessionIdNotFound, lsR_InvalidParameter);
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

    *pOutAvailableTime = pool_get(&_Users, userId)->availableTimePerDay;
  }

  return result;
}

lsResult replace_available_time(const size_t userId, const local_list<time_span_t, DaysPerWeek> availableTime)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    pool_get(&_Users, userId)->availableTimePerDay = availableTime;
  }

  _UserChangingStatus++;

  return result;
}

lsResult add_new_event(event evnt)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    size_t _unused;
    LS_DEBUG_ERROR_ASSERT(pool_add(&_Events, evnt, &_unused));
  }

  _EventChangingStatus++;

  return result;
}

lsResult get_current_events_from_user_id(const size_t userId, _Out_ local_list<event_info, maxEventsPerUserPerDay> *pOutCurrentEvents)
{
  lsResult result = lsR_Success;
  
  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    const user *pUser = pool_get(&_Users, userId);
    
    for (const auto &_item : pUser->tasksForCurrentDay)
    {
      event *pEvent = pool_get(&_Events, _item);

      event_info info;
      info.id = _item;
      strncpy(info.name, pEvent->name, LS_ARRAYSIZE(info.name));
      info.durationInMinutes = minutes_from_time_span(pEvent->durationTimeSpan);
     
      LS_ERROR_CHECK(local_list_add(pOutCurrentEvents, info));
    }
  }

epilogue:
  return result;
}

lsResult get_completed_events_for_current_day(const size_t userId, _Out_ local_list<event_info, maxEventsPerUserPerDay> *pOutCompletedTasks)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    user usr = *pool_get(&_Users, userId);
    
    for (const auto &_item : usr.completedTasksForCurrentDay)
    {
      event evnt = *pool_get(&_Events, _item);

      event_info info;
      info.id = _item;
      info.durationInMinutes = minutes_from_time_span(evnt.durationTimeSpan);
      strncpy(info.name, evnt.name, LS_ARRAYSIZE(info.name));

      LS_ERROR_CHECK(local_list_add(pOutCompletedTasks, info));
    }
  }

epilogue:
  return result;
}

lsResult replace_task(const size_t id, const event evnt)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);
    
    LS_DEBUG_ERROR_ASSERT(pool_insertAt(&_Events, evnt, id, true));
    //*pool_get(&_Events, id) = evnt; // what's wrong with this?
  }

  _EventChangingStatus++;

  return result;
}

lsResult set_event_last_modified_time(const size_t eventId)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    event *pEvent = pool_get(&_Events, eventId);
    pEvent->lastCompletedTime = get_current_time();
  }

  _EventChangingStatus++;

  return result;
}

lsResult add_completed_task(const size_t eventId, const size_t userId)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    user *pUser = pool_get(&_Users, userId);
    LS_ERROR_CHECK(local_list_add(&pUser->completedTasksForCurrentDay, eventId));
  }

epilogue:
  return result;
}

lsResult search_events(const char *searchTerm, _Out_ local_list<event_info, maxSearchResults> *pOutSearchResults)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_item : _Events)
    {
      if (strstr(_item.pItem->name, searchTerm) != nullptr)
      {
        event_info info;
        info.id = _item.index;
        strncpy(info.name, _item.pItem->name, LS_ARRAYSIZE(info.name));
        info.durationInMinutes = minutes_from_time_span(_item.pItem->durationTimeSpan);

        LS_ERROR_CHECK(local_list_add(pOutSearchResults, info));
      }
    }    
  }

epilogue:
  return result;
}

lsResult search_events_by_user(const size_t userId, const char *searchTerm, _Out_ local_list<event_info, maxSearchResults> *pOutSearchResults)
{
  lsResult result = lsR_Success;

  local_list<size_t, maxSearchResults> eventIds;
  LS_ERROR_CHECK(get_all_event_ids_for_user(userId, &eventIds));

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &_item : eventIds)
    {
      event evnt = *pool_get(&_Events, _item);

      if (strstr(evnt.name, searchTerm) != nullptr)
      {
        event_info info;
        info.id = _item;
        strncpy(info.name, evnt.name, LS_ARRAYSIZE(info.name));
        info.durationInMinutes = minutes_from_time_span(evnt.durationTimeSpan);

        LS_ERROR_CHECK(local_list_add(pOutSearchResults, info));
      }
    }
  }

epilogue:
  return result;
}

lsResult search_users(const char *searchTerm, _Out_ local_list<user_info, maxSearchResults> *pOutSearchResults)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_item : _Users)
    {
      if (strstr(_item.pItem->username, searchTerm))
      {
        user_info info;
        info.id = _item.index;
        strncpy(info.name, _item.pItem->username, LS_ARRAYSIZE(info.name));

        LS_ERROR_CHECK(local_list_add(pOutSearchResults, info));
      }
    }
  }

epilogue:
  return result;
}

lsResult get_all_event_ids_for_user(const size_t userId, _Out_ local_list<size_t, maxSearchResults> *pOutEventIds)
{
  lsResult result = lsR_Success;

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_item : _Events)
    {
      for (const auto &_id : _item.pItem->userIds)
      {
        if (_id == userId)
          LS_ERROR_CHECK(local_list_add(pOutEventIds, _item.index));
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

    *pEvent = *pool_get(&_Events, id);
  }

  return result;
}

bool check_for_user_name_duplication(const char *username)
{
  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_item : _Users)
      if ((strncmp(_item.pItem->username, username, LS_ARRAYSIZE(_item.pItem->username)) == 0))
        return false;
  }
  return true;
}

time_point_t get_current_time()
{
  return (time_point_t)time(nullptr);
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
