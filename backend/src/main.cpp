#include <stdio.h>
#include <exception>

#define ASIO_STANDALONE 1
#define ASIO_NO_EXCEPTIONS 1

#define SCHEDD_LOCALHOST
#define SCHEDD_HOSTNAME "https://hostname_not_configured"

namespace asio
{
  namespace detail
  {
    template <typename Exception>
    void throw_exception(const Exception &e)
    {
      printf("Exception thrown: %s.\n", e.what());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4702) // unreachable (somewhere in json.h)
#endif

//////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning (push, 0)
#endif
#include "crow.h"
#include "crow/middlewares/cors.h"
#ifdef _MSC_VER
#pragma warning (pop)
#endif

//////////////////////////////////////////////////////////////////////////

#include "schedd.h"

//////////////////////////////////////////////////////////////////////////

crow::response handle_login(const crow::request &req);
crow::response handle_user_registration(const crow::request &req);
crow::response handle_task_creation_modification(const crow::request &req, const bool isCreation);
crow::response handle_user_schedule(const crow::request &req);
crow::response handle_event_search(const crow::request &req);
crow::response handle_event_completed(const crow::request &req);
crow::response handle_task_details(const crow::request &req);

//////////////////////////////////////////////////////////////////////////

std::atomic<bool> _IsRunning = true;
std::thread *pAsyncTasksThread = nullptr;

void async_tasks();

//////////////////////////////////////////////////////////////////////////

int32_t main(void)
{
  // TODO: Deserialize.

  user poepe;
  strncpy(poepe.username, "poepe", LS_ARRAYSIZE(poepe.username));
  const time_span_t pupusTime = time_span_from_minutes(120);
  for (size_t i = 0; i < DaysPerWeek; i++)
    local_list_add(&poepe.availableTimeInMinutesPerDay, pupusTime);

  add_new_user(poepe);

  crow::App<crow::CORSHandler> app;

  auto &cors = app.get_middleware<crow::CORSHandler>();
#ifndef SCHEDD_LOCALHOST
  cors.global().origin(SCHEDD_HOSTNAME);
#else
  cors.global().origin("*");
#endif

  CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_login(req); });
  CROW_ROUTE(app, "/registration").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_user_registration(req); });
  CROW_ROUTE(app, "/task-creation").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_task_creation_modification(req, true); });
  CROW_ROUTE(app, "/task-edit").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_task_creation_modification(req, false); });
  CROW_ROUTE(app, "/user-schedule").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_user_schedule(req); });
  CROW_ROUTE(app, "/task-search").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_event_search(req); });
  CROW_ROUTE(app, "/task-done").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_event_completed(req); });
  CROW_ROUTE(app, "/task").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_task_details(req); });

  pAsyncTasksThread = new std::thread(async_tasks);

  app.port(61919).multithreaded().run();

  _IsRunning = false;
}
//////////////////////////////////////////////////////////////////////////

void async_tasks()
{
  constexpr size_t WaitTimeSeconds = 20;

  while (true)
  {
    for (size_t i = 0; i < WaitTimeSeconds; i++) // To not lock up for 20 seconds.
    {
      if (!_IsRunning)
        return;

#ifdef _WIN32
      Sleep(1000);
#else
#fail Sleep Not Implemented.
#endif
    }

    // TODO: If Changed: Serialize.
    // TODO: If Changed: Reschedule.
  }
}

//////////////////////////////////////////////////////////////////////////

crow::response handle_login(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("username"))
    return crow::response(crow::status::BAD_REQUEST);

  const std::string &username = body["username"].s();

  int32_t sessionId;
  
  if (LS_FAILED(assign_session_token(username.c_str(), &sessionId)))
    return crow::response(crow::status::UNAUTHORIZED);

  crow::json::wvalue ret;
  ret["session_id"] = sessionId;

  return crow::response(crow::status::OK, ret);
}

crow::response handle_user_registration(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("username") || !body.has("availableTime"))
    return crow::response(crow::status::BAD_REQUEST);

  user usr;

  // Username
  const std::string &username = body["username"].s();
  if (username.length() + 1 > LS_ARRAYSIZE(usr.username) || username.length() == 0)
    return crow::response(crow::status::BAD_REQUEST);

  if (!(check_for_user_name_duplication(username.c_str())))
    return crow::response(crow::status::BAD_REQUEST);

  strncpy(usr.username, username.c_str(), LS_ARRAYSIZE(usr.username));

  // Available Time per Day
  for (const auto &_item : body["availableTime"])
  {
    if (_item.i() < 0 || _item.i() > 24 * 60)
      return crow::response(crow::status::BAD_REQUEST);

    lsAssert(_item.i() >= 0);
    local_list_add(&usr.availableTimeInMinutesPerDay, time_span_from_minutes(_item.i()));
  }

  if (LS_FAILED(add_new_user(usr)))
    return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  int32_t sessionId;
  if (LS_FAILED(assign_session_token(username.c_str(), &sessionId)))
    return crow::response(crow::status::UNAUTHORIZED);

  if (LS_FAILED(set_events_for_user(sessionId)))
    return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  crow::json::wvalue ret;
  ret["session_id"] = sessionId;

  return crow::response(crow::status::OK, ret);
}

crow::response handle_task_creation_modification(const crow::request &req, const bool isCreation)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId") || !body.has("name") || !body.has("duration") || !body.has("possibleExecutionDays") || !body.has("repetitionTimeSpan") || !body.has("weight") || !body.has("weightFactor"))
    return crow::response(crow::status::BAD_REQUEST);

  const int32_t sessionId = (int32_t)body["sessionId"].i();
  const std::string &eventName = body["name"].s();
  const uint64_t duration = body["duration"].i();
  const uint64_t repetitionTimeSpan = body["repetitionTimeSpan"].i();
  const uint64_t weight = body["weight"].i();
  const uint64_t weightFactor = body["weightFactor"].i();
  local_list<bool, 7> executionDays;
  
  for (const auto &b : body["possibleExecutionDays"])
    if (LS_FAILED(local_list_add(&executionDays, b.b())))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  // TODO: Possibilty to add other users

  event evnt;
  size_t userId;

  if (LS_FAILED(get_user_id_from_session_id(sessionId, &userId)))
    return crow::response(crow::status::BAD_REQUEST);
  
  if(LS_FAILED(local_list_add(&evnt.userIds, userId)))
    return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  if (eventName.length() == 0 || eventName.length() > LS_ARRAYSIZE(evnt.name))
    return crow::response(crow::status::BAD_REQUEST);

  strncpy(evnt.name, eventName.c_str(), LS_ARRAYSIZE(evnt.name));

  if (repetitionTimeSpan < 0)
    return crow::response(crow::status::BAD_REQUEST);

  evnt.repetitionTimeSpan = time_span_from_days(repetitionTimeSpan);

  if (weight < 0 || weight > 100)
    return crow::response(crow::status::BAD_REQUEST);

  evnt.weight = weight;

  if (weightFactor < 0 || weightFactor > 100)
    return crow::response(crow::status::BAD_REQUEST);

  evnt.weightGrowthFactor = weightFactor;

  uint8_t executionDayFlags = wF_None;

  for (size_t i = 0; i < DaysPerWeek; i++)
    executionDayFlags |= ((uint8_t)executionDays[i] << i);

  evnt.possibleExecutionDays = (weekday_flags)executionDayFlags;

  if (duration > 24 * 60 || duration < 1)
    return crow::response(crow::status::BAD_REQUEST);

  evnt.durationTimeSpan = time_span_from_minutes(duration);

  evnt.creationTime = get_current_time();

  if (isCreation)
  {
    if (LS_FAILED(add_new_event(evnt)))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
  }
  else
  {
    if (!body.has("id"))
      return crow::response(crow::status::BAD_REQUEST);

    size_t id = body["id"].i();

    evnt.lastModifiedTime = get_current_time();

    if (LS_FAILED(replace_task(id, evnt)))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
  }

  crow::json::wvalue ret;
  ret["success"] = true;

  return crow::response(crow::status::OK, ret);
}

crow::response handle_user_schedule(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId"))
    return crow::response(crow::status::BAD_REQUEST);

  int32_t sessionId = (int32_t)body["sessionId"].i();

  local_list<event_info, maxEventsPerUserPerDay> currentTasks;
  if (LS_FAILED(get_current_events_from_session_id(sessionId, &currentTasks)))
    return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  crow::json::wvalue ret = crow::json::rvalue(crow::json::type::List);

  for (int8_t i = 0; i < currentTasks.count; i++)
  {
    ret[i]["name"] = currentTasks[i].name;
    ret[i]["duration"] = currentTasks[i].durationInMinutes;
    ret[i]["id"] = currentTasks[i].id;
  }

  return crow::response(crow::status::OK, ret);
}

crow::response handle_event_search(const crow::request &req) // coc? what bad stuff is lino doing?
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId") || !body.has("input"))
    return crow::response(crow::status::BAD_REQUEST);

  const int32_t sessionId = (int32_t)body["sessionId"].i();
  const std::string &input = body["input"].s();

  size_t userId;
  if (LS_FAILED(get_user_id_from_session_id(sessionId, &userId)))
    return crow::response(crow::status::BAD_REQUEST);

  local_list<event_info, maxSearchResults> searchResults; // TODO: get event_infos in a for loop for less memory usage?
  if (LS_FAILED(search_events_by_user(userId, input.c_str(), &searchResults)))
    return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  crow::json::wvalue ret = crow::json::rvalue(crow::json::type::List);

  for (int8_t i = 0; i < searchResults.count; i++)
  {
    ret[i]["name"] = searchResults[i].name;
    ret[i]["duration"] = searchResults[i].durationInMinutes;
    ret[i]["id"] = searchResults[i].id;
  }

  return crow::response(crow::status::OK, ret);
}

crow::response handle_event_completed(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId") || !body.has("taskId"))
    return crow::response(crow::status::BAD_REQUEST);

  const size_t eventId = body["taskId"].i();
  const int32_t sessionId = (int32_t)body["sessionId"].i();

  if (LS_FAILED(set_event_last_modified_time(eventId)))
    return crow::response(crow::status::BAD_REQUEST);

  size_t userId;
  if (LS_FAILED(get_user_id_from_session_id(sessionId, &userId)))
    return crow::response(crow::status::BAD_REQUEST);

  // TODO: Do we want to remove the task from tasks for today list?
  if (LS_FAILED(add_completed_task(eventId, userId)))
    return crow::response(crow::status::BAD_REQUEST);

  local_list <event_info, maxEventsPerUserPerDay> completedTasks;

  // get this with schedule if even at all
  if (LS_FAILED(get_completed_events_for_current_day(userId, &completedTasks))) // TODO: get event_infos in a for loop for less memory usage?
    return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  crow::json::wvalue ret;
  ret["success"] = true;

  return crow::response(crow::status::OK, ret);
}

crow::response handle_task_details(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("taskId"))
    return crow::response(crow::status::BAD_REQUEST);

  const size_t taskId = body["taskId"].i();

  event evnt;
  if (LS_FAILED(get_event(taskId, &evnt)))
    return crow::response(crow::status::BAD_REQUEST);

  crow::json::wvalue ret;

  ret["name"] = evnt.name;

  ret["duration"] = evnt.durationTimeSpan / 60;

  for (int8_t i = 0; i < DaysPerWeek; i++)
    ret["executionDays"][i] = !!(evnt.possibleExecutionDays & (1 << i));

  ret["repetition"] = evnt.repetitionTimeSpan / (60 * 60 * 24);
  ret["weight"] = evnt.weight;
  ret["weightFactor"] = evnt.weightGrowthFactor;

  return crow::response(crow::status::OK, ret);
}
