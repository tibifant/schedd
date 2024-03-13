#pragma once

#include "core.h"

#include "local_list.h"
#include "pool.h"

constexpr size_t maxUsersPerEvent = 16;

typedef uint64_t time_point_t;
typedef uint64_t time_span_t;

enum weekday_flags : uint8_t
{
  wF_None = 0,
  wF_Monday = 1 << 0,
  wF_Tuesday = 1 << 1,
  wF_Wendesday = 1 << 2,
  wF_Thursday = 1 << 3,
  wF_Friday = 1 << 4,
  wF_Saturday = 1 << 5,
  wF_Sunday = 1 << 6,
  wF_All = (1 << 7) - 1,
};

struct event
{
  char name[256];
  uint64_t duration; // which datatype is suitable, could possibly have a max value of 24h * 60min
  local_list<uint64_t, maxUsersPerEvent> userIds;
  size_t weight, weightGrowthFactor;
  weekday_flags possibleExecutionDays; // 1 bit for each day + 1 extra
  time_span_t repetitionTimeSpan; // if 0: don't repeat!
  time_point_t creationTime, lastCompletedTime, lastModifiedTime;
};

struct session_token
{
  int64_t sessionId;
};

constexpr size_t maxUserAmount = 64;

struct user_info
{
  int64_t sessionId;
  uint64_t userId;
};

constexpr size_t maxEventsPerUserPerDay = 32;
constexpr size_t maxSessionsPerUser = 8;

struct user
{
  char username[256];
  local_list<session_token, maxSessionsPerUser> sessionIds; // how do i know if it's in use? second list with acrive tokens? bool in sessio_token `is_in_use`?
  uint64_t availableTimeInMinutesPerDay[7];
  local_list<uint8_t, maxEventsPerUserPerDay> tasksForCurrentDay; // index of the event or else array of events
  local_list<uint8_t, maxEventsPerUserPerDay> completedTasksForCurrentDay; // index of the event or else array if events
};

lsResult assign_session_token(const char *username, _Out_ int64_t *pOutSessionId);
lsResult create_new_user(const char *username);

// what day is it?

// events:
// mark as done
// edit event
// create new event
