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
      if (strncmp(_item.pItem->userName, username, LS_ARRAYSIZE(_item.pItem->userName)) == 0) // if name matches.
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

uint64_t create_new_user(const char *username)
{
  user newUser;
  strncpy(newUser.userName, username, LS_ARRAYSIZE(newUser.userName));

  uint64_t userId;
  {
    std::scoped_lock lock(_ThreadLock);
    LS_DEBUG_ERROR_ASSERT(pool_add(&_Users, newUser, &userId));
  }

  return userId;
}
