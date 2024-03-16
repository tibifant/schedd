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

lsResult assign_session_token(const char *username, _Out_ int64_t *pOutSessionId)
{
  lsResult result = lsR_Success;

  // Scope Lock.
  {
    std::scoped_lock lock(_ThreadLock);

    uint64_t userId = 0; // If no value is assigned there's a warning, but we won't ever use this uninitilaized as we would not pass the `error_if`.
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
      *pOutSessionId = lsGetRand(); // we should check if this is unique

      // Check for sessionId duplicates
      for (const auto &_item : usr.sessionTokens)
        while (_item.sessionId == *pOutSessionId)
          *pOutSessionId = lsGetRand();

      session_token token;
      token.sessionId = *pOutSessionId;

      // if user already has maximum amount of userids, replace first
      if (usr.sessionTokens.count == maxSessionsPerUser)
      {
        usr.sessionTokens[0] = token;
      }
      else
      {
        local_list_add(&usr.sessionTokens, token);
        
        // Add User Info to List
        user_info userInfo;
        userInfo.sessionId = *pOutSessionId;
        userInfo.userId = userId;

        local_list_add(&_UserInfo, userInfo);
      }
    }
  }

  _UserChangingStatus++;

 epilogue:
  return result;
}

lsResult create_new_user(const char *username)
{
  lsResult result = lsR_Success;

  user newUser;

  LS_ERROR_IF(strlen(username) + 1 > LS_ARRAYSIZE(newUser.username), lsR_ArgumentOutOfBounds);
  LS_ERROR_IF(strlen(username) == 0, lsR_InvalidParameter);

  // Scope Lock
  {
    std::scoped_lock lock(_ThreadLock);

    // Check for duplication of usernames
    for (const auto &&_item : _Users)
    {
      LS_ERROR_IF((strncmp(_item.pItem->username, username, LS_ARRAYSIZE(_item.pItem->username)) == 0), lsR_InvalidParameter); 
    }

    // Set username of new user.
    strncpy(newUser.username, username, LS_ARRAYSIZE(newUser.username));

    // Add user to pool
    size_t _unused;
    LS_DEBUG_ERROR_ASSERT(pool_add(&_Users, newUser, &_unused));
  }

  _UserChangingStatus++;

epilogue:
  return result;
}

lsResult get_user_id_from_session_id(const int64_t sessionId, _Out_ uint64_t *pUserId)
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

time_point_t get_current_time()
{
  return (time_point_t)time(nullptr);
}

time_span_t time_span_from_days(const size_t days)
{
  constexpr int64_t day_to_unix_timestamp = 60 * 60 * 24;

  return (time_span_t)(day_to_unix_timestamp * days);
}
