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
crow::response handle_task_creation(const crow::request &req);

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

  create_new_user("poepe");

  CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_login(req); });
  CROW_ROUTE(app, "/registration").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_user_registration(req); });
  CROW_ROUTE(app, "/task-creation").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_task_creation(req); });

  app.port(61919).multithreaded().run();
}
//////////////////////////////////////////////////////////////////////////

crow::response handle_login(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("username"))
    return crow::response(crow::status::BAD_REQUEST);

  const std::string &username = body["username"].s();

  int64_t sessionId;
  
  if (LS_FAILED(assign_session_token(username.c_str(), &sessionId)))
    return crow::response(crow::status::UNAUTHORIZED);

  crow::json::wvalue ret;
  ret["session_id"] = sessionId;

  return crow::response(crow::status::OK, ret);
}

crow::response handle_user_registration(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("username"))
    return crow::response(crow::status::BAD_REQUEST);

  const std::string &username = body["username"].s();

  if (LS_FAILED(create_new_user(username.c_str())))
    return crow::response(crow::status::FORBIDDEN);

  int64_t sessionId;

  if (LS_FAILED(assign_session_token(username.c_str(), &sessionId)))
    return crow::response(crow::status::UNAUTHORIZED);

  crow::json::wvalue ret;
  ret["session_id"] = sessionId;

  return crow::response(crow::status::OK, ret);
}

crow::response handle_task_creation(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId") || !body.has("name") || !body.has("duration") || !body.has("possibleExecutionDays") || !body.has("repetitionTimeSpan") || !body.has("weight"))
    return crow::response(crow::status::BAD_REQUEST);

  const int64_t sessionId = body["sessionId"].i();
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
  uint64_t userId;

  if (LS_FAILED(get_user_id_from_session_id(sessionId, &userId)))
    return crow::response(crow::status::BAD_REQUEST);

  local_list_add(&evnt.userIds, userId);

  if (eventName.length() == 0 || eventName.length() > LS_ARRAYSIZE(evnt.name))
    return crow::response(crow::status::BAD_REQUEST);

  strncpy(evnt.name, eventName.c_str(), LS_ARRAYSIZE(evnt.name));

  if (duration > 24 * 60 || duration < 1) // this should also not exceed the maximum time of the user for the possible execution days
    return crow::response(crow::status::BAD_REQUEST);

  evnt.duration = duration;

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

  constexpr size_t DaysPerWeek = 7;

  for (size_t i = 0; i < DaysPerWeek; i++)
    executionDayFlags |= ((uint8_t)executionDays[i] << i);

  evnt.possibleExecutionDays = (weekday_flags)executionDayFlags;

  evnt.creationTime = get_current_time();

  if(LS_FAILED(add_new_task(evnt)))
    return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  crow::json::wvalue ret;
  ret["success"] = true;

  return crow::response(crow::status::OK, ret);
}
