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

crow::response handle_response_A(const crow::request &req);
crow::response handle_login(const crow::request &req);

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

  CROW_ROUTE(app, "/handle_response_A").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_response_A(req); });
  CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_login(req); });

  app.port(61919).multithreaded().run();
}
//////////////////////////////////////////////////////////////////////////

crow::response handle_response_A(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("param_A") || !body.has("param_B"))
    return crow::response(crow::status::BAD_REQUEST);

  const auto &param_A_string = body["param_A"].s();
  const uint64_t param_B_integer = body["param_B"].i();

  (void)param_A_string;
  (void)param_B_integer;

  crow::json::wvalue ret;
  ret["response_param"] = "response_param_value";

  return crow::response(crow::status::OK, ret);
}

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
