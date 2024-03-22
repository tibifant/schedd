#include "schedd.h"

#include <mutex>
#include <time.h>

static std::mutex _ThreadLock;

static pool<user> _Users;
static local_list<user_info, maxUserAmount *maxSessionsPerUser> _UserInfo;

static pool<event> _Events;

static std::atomic<uint64_t> _UserChangingStatus = 0;
static std::atomic<uint64_t> _EventChangingStatus = 0;

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
      *pOutSessionId = (int32_t)lsGetRand(); // we should check if this is unique

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
        user_info userInfo;
        userInfo.sessionId = *pOutSessionId;
        userInfo.userId = userId;
        local_list_add(&_UserInfo, userInfo); // TODO: Either cap the amount of maximum users or handle having no free slot for another userInfo!
      }
    }
  }

  _UserChangingStatus++;

 epilogue:
  return result;
}

lsResult create_new_user(const char *username, const local_list<uint64_t, DaysPerWeek> *pAvailableTimePerDay)
{
  lsResult result = lsR_Success;

  user usr;

  LS_ERROR_IF(strlen(username) + 1 > LS_ARRAYSIZE(usr.username), lsR_ArgumentOutOfBounds);
  LS_ERROR_IF(strlen(username) == 0, lsR_InvalidParameter);
  // aivailableTimePerDay gets checked at the function call. 

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    // Check for duplication of usernames
    for (const auto &&_item : _Users)
      LS_ERROR_IF((strncmp(_item.pItem->username, username, LS_ARRAYSIZE(_item.pItem->username)) == 0), lsR_InvalidParameter);

    // Set username and available time of new user.
    strncpy(usr.username, username, LS_ARRAYSIZE(usr.username));

    lsAssert(LS_ARRAYSIZE(usr.availableTimeInMinutesPerDay) == pAvailableTimePerDay->count);
    lsMemcpy(&usr.availableTimeInMinutesPerDay, &pAvailableTimePerDay->values, LS_ARRAYSIZE(usr.availableTimeInMinutesPerDay));

    // Add user to pool
    size_t _unused;
    LS_DEBUG_ERROR_ASSERT(pool_add(&_Users, usr, &_unused));
  }

  _UserChangingStatus++;

epilogue:
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

lsResult add_new_task(event evnt)
{
  lsResult result = lsR_Success;

  {
    std::scoped_lock lock(_ThreadLock);

    size_t _unused;
    LS_ERROR_CHECK(pool_add(&_Events, &evnt, &_unused));
  }

  _EventChangingStatus++;

epilogue:
  return result;
}

lsResult get_current_events_from_session_id(const int32_t sessionId, _Out_ local_list<event_info, maxEventsPerUserPerDay> *pOutCurrentEvents)
{
  lsResult result = lsR_Success;

  size_t userId;
  LS_ERROR_CHECK(get_user_id_from_session_id(sessionId, &userId));
  
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
      info.duration = pEvent->duration;
     
      local_list_add(pOutCurrentEvents, &info);
    }
  }

epilogue:
  return result;
}

lsResult set_events_for_user(const int32_t sessionId)
{
  lsResult result = lsR_Success;

  size_t userId;
  LS_ERROR_CHECK(get_user_id_from_session_id(sessionId, &userId));

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);
    
    event poepePutzt;
    strncpy(poepePutzt.name, "poepe muss putzen", LS_ARRAYSIZE(poepePutzt.name));
    poepePutzt.duration = 20;
    event poepeKocht;
    strncpy(poepeKocht.name, "poepe chefkoch", LS_ARRAYSIZE(poepePutzt.name));
    poepeKocht.duration = 100;

    size_t putzEventId;
    pool_add(&_Events, poepePutzt, &putzEventId);
    size_t kochEventId;
    pool_add(&_Events, poepeKocht, &kochEventId);

    user *pUser = pool_get(&_Users, userId);
    local_list_add(&pUser->tasksForCurrentDay, putzEventId);
    local_list_add(&pUser->tasksForCurrentDay, kochEventId);
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
    
    *pool_get(&_Events, id) = evnt;
  }

  _EventChangingStatus++;

  return result;
}

//bool check_event_duration_compatibilty(size_t userId, uint64_t duration, weekday_flags executionDays)
//{
//  bool isCompatible = false;
//
//  // Scope Lock
//  {
//    std::scoped_lock lock(_ThreadLock);
//
//    user *pUser = pool_get(&_Users, userId);
//
//    for (size_t i = 0; i < LS_ARRAYSIZE(pUser->availableTimeInMinutesPerDay); i++)
//    {
//      if (executionDays & (1 << i))
//      {
//        if (*pUser->availableTimeInMinutesPerDay >= duration)
//        {
//          isCompatible = true; // It is enough if one day is compatible
//          break;
//        }
//      }
//    }
//  }
//
//  return isCompatible;
//}

time_point_t get_current_time()
{
  return (time_point_t)time(nullptr);
}

time_span_t time_span_from_days(const size_t days)
{
  constexpr int64_t day_to_unix_timestamp = 60 * 60 * 24;

  return (time_span_t)(day_to_unix_timestamp * days);
}
