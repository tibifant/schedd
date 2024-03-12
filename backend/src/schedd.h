#pragma once

#include <stdint.h>
#include <inttypes.h>

#include "local_list.h"

constexpr size_t maxUsersPerEvent = 16;
constexpr size_t maxEventNameLength = 64;

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
  char name[maxEventNameLength]; // think about length
  uint64_t duration; // which datatype is suitable, could possibly have a max value of 24h * 60min
  local_list<uint64_t, maxUsersPerEvent> userIds;
  size_t weight, weightGrowthFactor;
  weekday_flags possibleExecutionDays; // 1 bit for each day + 1 extra
  time_span_t repetitionTimeSpan; // if 0: don't repeat!
  time_point_t creationTime, lastCompletedTime, lastModifiedTime;
};

constexpr size_t maxEventsPerUserPerDay = 32;
constexpr size_t maxUsernameLength = 32;
constexpr size_t maxSessionsPerUser = 32;

struct user
{
  char userName[maxUsernameLength];
  local_list<uint64_t, maxSessionsPerUser> sessionIds;
  uint64_t availableTimeInMinutesPerDay[7];
  local_list<uint8_t, maxEventsPerUserPerDay> tasksForCurrentDay; // index of the event or else array if events
  local_list<uint8_t, maxEventsPerUserPerDay> completedTasksForCurrentDay; // index of the event or else array if events
};

// userids in pool

// what day is it?

// events:
// mark as done
// edit event
// create new event
