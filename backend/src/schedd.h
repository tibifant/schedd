#pragma once

#include "core.h"

#include "local_list.h"
#include "pool.h"
#include "small_list.h"

extern std::atomic<size_t> _UserChangingStatus;
extern std::atomic<size_t> _EventChangingStatus;
extern std::atomic<size_t> _ExplicitlyRequestsReschedule;

void reschedule_events_for_user(const size_t userId); // Assumes mutex lock

constexpr size_t DaysPerWeek = 7;

constexpr size_t maxUsersPerEvent = 16;

typedef uint64_t time_point_t;
typedef int64_t time_span_t;

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
  time_span_t durationTimeSpan;
  local_list<size_t, maxUsersPerEvent> userIds;
  uint64_t weight, weightGrowthFactor;
  weekday_flags possibleExecutionDays; // 1 bit for each day + 1 extra
  time_span_t repetitionTimeSpan; // if 0: don't repeat!
  time_point_t creationTime, lastCompletedTime, lastModifiedTime; // if lastCompletedTime == 0: hasn;t been executed so far.
};

struct session_token
{
  int32_t sessionId;
};

constexpr size_t maxUserAmount = 64;

struct user_id_info
{
  int32_t sessionId;
  size_t userId;
};

constexpr size_t maxEventsPerUserPerDay = 32;
constexpr size_t maxSessionsPerUser = 8;

struct user
{
  char username[256];
  local_list<session_token, maxSessionsPerUser> sessionTokens;
  local_list<time_span_t, DaysPerWeek> availableTimePerDay;
  local_list<size_t, maxEventsPerUserPerDay> tasksForCurrentDay; // index of the event
  local_list<size_t, maxEventsPerUserPerDay> completedTasksForCurrentDay; // index of the event
};

extern pool<user> _Users;
extern pool<event> _Events;

lsResult assign_session_token(const char *username, _Out_ int32_t *pOutSessionId);
lsResult add_new_user(const user usr);
lsResult add_new_event(event evnt);
lsResult get_user_id_from_session_id(const int32_t sessionId, _Out_ size_t *pUserId);

struct event_info
{
  size_t id;
  size_t durationInMinutes;
  char name[256];
};

struct user_info
{
  size_t id;
  char name[256];
};

lsResult get_user_info(const size_t userId, _Out_ user_info *pOutInfo);

lsResult get_current_events_from_user_id(const size_t userId, _Out_ local_list<event_info, maxEventsPerUserPerDay> *pOutCurrentEvents);
lsResult get_completed_events_for_current_day(const size_t userId, _Out_ local_list<event_info, maxEventsPerUserPerDay> *pOutCompletedTasks);

constexpr size_t maxSearchResults = 32;
lsResult search_events(const char *searchTerm, _Out_ local_list<event_info, maxSearchResults> *pOutSearchResults);
lsResult search_events_by_user(const size_t userId, const char *searchTerm, _Out_ local_list<event_info, maxSearchResults> *pOutSearchResults);
lsResult search_users(const char *searchTerm, _Out_ local_list<user_info, maxSearchResults> *pOutSearchResults);

lsResult get_all_event_ids_for_user(const size_t userId, _Out_ local_list<size_t, maxSearchResults> *pOutEventIds);

lsResult replace_task(const size_t id, const event evnt);
lsResult set_event_last_modified_time(const size_t eventId);
lsResult add_completed_task(const size_t eventId, const size_t userId);
lsResult get_event(const size_t taskId, _Out_ event *pEvent);

bool check_for_user_name_duplication(const char *username);

time_point_t get_current_time();
time_span_t time_span_from_days(const size_t days);
size_t days_from_time_span(const time_span_t timeSpan);
time_span_t time_span_from_minutes(const size_t minutes);
size_t minutes_from_time_span(const time_span_t timeSpan);
