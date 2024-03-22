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

//////////////////////////////////////////////////////////////////////////

int32_t main(void)
{
  crow::App<crow::CORSHandler> app;

  auto &cors = app.get_middleware<crow::CORSHandler>();
#ifndef SCHEDD_LOCALHOST
  cors.global().origin(SCHEDD_HOSTNAME);
#else
  cors.global().origin("*");
#endif

  local_list<uint64_t, DaysPerWeek> poepesAvailableHours;
  constexpr uint64_t pupusTime = 60 * 3;
  for (size_t i = 0; i < DaysPerWeek; i++)
    local_list_add(&poepesAvailableHours, &pupusTime);

  create_new_user("poepe", &poepesAvailableHours);

  CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_login(req); });
  CROW_ROUTE(app, "/registration").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_user_registration(req); });
  CROW_ROUTE(app, "/task-creation").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_task_creation_modification(req, true); });
  CROW_ROUTE(app, "/task-edit").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_task_creation_modification(req, false); });
  CROW_ROUTE(app, "/user-schedule").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_user_schedule(req); });

  app.port(61919).multithreaded().run();
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

  const std::string &username = body["username"].s();
  local_list<uint64_t, DaysPerWeek> availableTime;

  for (const auto time : body["availableTime"])
  {
    if (time.i() < 0 || time.i() > 24 * 60)
      return crow::response(crow::status::BAD_REQUEST);

    lsAssert(time.i() >= 0);
    local_list_add(&availableTime, (uint64_t)time.i());
  }

  if (LS_FAILED(create_new_user(username.c_str(), &availableTime)))
    return crow::response(crow::status::FORBIDDEN);

  int32_t sessionId;
  if (LS_FAILED(assign_session_token(username.c_str(), &sessionId)))
    return crow::response(crow::status::UNAUTHORIZED);

  crow::json::wvalue ret;
  ret["session_id"] = sessionId;

  return crow::response(crow::status::OK, ret);
}

crow::response handle_task_creation_modification(const crow::request &req, const bool isCreation)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId") || !body.has("name") || !body.has("duration") || !body.has("possibleExecutionDays") || !body.has("repetitionTimeSpan") || !body.has("weight"))
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
      return crow::response(crow::status::BAD_REQUEST);

  // TODO: Possibilty to add other users

  event evnt;
  size_t userId;

  if (LS_FAILED(get_user_id_from_session_id(sessionId, &userId)))
    return crow::response(crow::status::BAD_REQUEST);
  
  local_list_add(&evnt.userIds, userId);

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

  evnt.duration = duration;

  evnt.creationTime = get_current_time();

  if (isCreation)
  {
    if (LS_FAILED(add_new_task(evnt)))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
  }
  else
  {
    if (!body.has("id"))
      return crow::response(crow::status::BAD_REQUEST);

    size_t id = body["id"].i();

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

  set_events_for_user(sessionId);

  local_list<event_info, maxEventsPerUserPerDay> currentTasks;
  if (LS_FAILED(get_current_events_from_session_id(sessionId, &currentTasks)))
    return crow::response(crow::status::BAD_REQUEST);

  crow::json::wvalue ret;
  for (int8_t i = 0; i < currentTasks.count; i++)
  {
    ret[i]["name"] = currentTasks[i].name;
    ret[i]["duration"] = currentTasks[i].duration;
    ret[i]["id"] = currentTasks[i].id;
  }

  return crow::response(crow::status::OK, ret);
}
