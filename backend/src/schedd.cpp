#include "schedd.h"

#include <mutex>

static std::mutex _ThreadLock;
static pool<user> _Users;

lsResult assign_session_token(const char *username, _Out_ int64_t *pOutSessionId)
{
  lsResult result = lsR_Success;

  // Scope Lock.
  {
    std::scoped_lock lock(_ThreadLock);

    user User;
    bool userNotFound = true;

    // iterate pool checking for username
    for (const auto &&_item : _Users)
    {
      if (strncmp(_item.pItem->username, username, LS_ARRAYSIZE(_item.pItem->username)) == 0) // if name matches.
      {
        User = *_item.pItem;
        userNotFound = false;
        break;
      }
    }

    LS_ERROR_IF(userNotFound, lsR_InvalidParameter);

    // Assign Session Id.
    {
      *pOutSessionId = lsGetRand(); // we should check if this is unique
      session_token token;
      token.sessionId = *pOutSessionId;

      // if user already has maximum amount of userids, replace first
      if (User.sessionIds.count == maxSessionsPerUser)
        User.sessionIds[0] = token;
      else
        local_list_add(&User.sessionIds, token);
    }
  }

 epilogue:
  return result;
}

lsResult create_new_user(const char *username)
{
  lsResult result = lsR_Success;

  user newUser;

  {
    std::scoped_lock lock(_ThreadLock);

    for (const auto &&_item : _Users)
    {
      LS_ERROR_IF((strncmp(_item.pItem->username, username, LS_ARRAYSIZE(_item.pItem->username)) == 0), lsR_InvalidParameter); // if username matches existing one.
    }
  }

  strncpy(newUser.username, username, LS_ARRAYSIZE(newUser.username));

  uint64_t userId; // does anyone need this as return value from this function?
  {
    std::scoped_lock lock(_ThreadLock);
    LS_DEBUG_ERROR_ASSERT(pool_add(&_Users, newUser, &userId));
  }

epilogue:
  return result;
}
