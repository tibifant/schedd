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
#include "io.h"

//////////////////////////////////////////////////////////////////////////

crow::response handle_login(const crow::request &req);
crow::response handle_user_registration(const crow::request &req);
crow::response handle_task_creation_modification(const crow::request &req, const bool isCreation);
crow::response handle_user_schedule(const crow::request &req);
crow::response handle_user_search(const crow::request &req);
crow::response handle_event_search(const crow::request &req);
crow::response handle_event_completed(const crow::request &req, const bool needsReschdule);
crow::response handle_task_details(const crow::request &req);

//////////////////////////////////////////////////////////////////////////

std::atomic<bool> _IsRunning = true;
std::thread *pAsyncTasksThread = nullptr;
static std::mutex _ThreadLock;

void async_tasks();
void writeUsersPoolToFile();
void writeEventsPoolToFile();

void deserializeUsersPool();
void deserialzieEventsPool();

//////////////////////////////////////////////////////////////////////////

int32_t main(void)
{
  // TODO: Deserialize.
  deserializeUsersPool();
  deserialzieEventsPool();

  user poepe;
  strncpy(poepe.username, "poepe", LS_ARRAYSIZE(poepe.username));
  const time_span_t pupusTime = time_span_from_minutes(120);
  for (size_t i = 0; i < DaysPerWeek; i++)
    local_list_add(&poepe.availableTimePerDay, pupusTime);

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
  CROW_ROUTE(app, "/user-search").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_user_search(req); });
  CROW_ROUTE(app, "/task-done").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_event_completed(req, false); });
  CROW_ROUTE(app, "/task-done-reschedule").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_event_completed(req, true); });
  CROW_ROUTE(app, "/task").methods(crow::HTTPMethod::POST)([](const crow::request &req) { return handle_task_details(req); });
  // TODO: Option to change the user data

  pAsyncTasksThread = new std::thread(async_tasks);

  app.port(61919).multithreaded().run();

  _IsRunning = false;
}

//////////////////////////////////////////////////////////////////////////

void async_tasks()
{
  constexpr size_t WaitTimeSeconds = 20;

  size_t userChangingStatusBefore = 0;
  size_t eventChangingStatusBefore = 0;
  size_t explicitlyRequestedRescheduleBefore = 0;

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

    const size_t userChangingStatusCurrent = _UserChangingStatus;
    const size_t eventChangingStatusCurrent = _EventChangingStatus;
    const size_t explicitlyRequestedRescheduleCurrent = _ExplicitlyRequestsReschedule;
    bool needsReschdeule = explicitlyRequestedRescheduleBefore < explicitlyRequestedRescheduleCurrent;

    // If Changed: Serialize. Reschedule.
    if (userChangingStatusBefore < userChangingStatusCurrent)
    {      
      // Serialize _Users pool
      writeUsersPoolToFile();
      
      needsReschdeule = true;
    }
    
    if (eventChangingStatusBefore < eventChangingStatusCurrent)
    {
      // Serialize _Events pool
      writeEventsPoolToFile();
    
      needsReschdeule = true;
    }
    
    if (needsReschdeule)
    {
      // Reschedule
      {
        std::scoped_lock lock(_ThreadLock);
    
        for (const auto &&_item : _Users)
          reschedule_events_for_user(_item.index); // just all users I guess...
      }
    }

    userChangingStatusBefore = userChangingStatusCurrent;
    eventChangingStatusBefore = eventChangingStatusCurrent;
    explicitlyRequestedRescheduleBefore = explicitlyRequestedRescheduleCurrent;
  }
}
  
//////////////////////////////////////////////////////////////////////////

const char *_Index = "index";
const char *_Username = "username";
const char *_AvailableTimePerDay = "availableTimePerDay";
const char *_CompletedTasks = "completedTasksForCurrentDay";
const char *_FileNameUsers = "userspool.json";

const char *_Name = "name";
const char *_DurationTimeSpan = "durationTimeSpan";
const char *_UsersIds = "userIds";
const char *_Weight = "weight";
const char *_WeightFactor = "weightGrowthFactor";
const char *_PossibleExecutionDays = "possibleExecutionDays";
const char *_RepetitionTimeSpan = "repetitionTimeSpan";
const char *_LastCompletedTime = "lastCompletedTime";
const char *_LastModifiedTime = "lastModifiedTime";
const char *_CreationTime = "creationTime";
const char *_FileNameEvents = "eventspool.json";

//////////////////////////////////////////////////////////////////////////

void writeUsersPoolToFile()
{
  std::string stringOut;

  // Mutex Lock
  {
    std::scoped_lock lock(_ThreadLock);
    int idx = 0;
    crow::json::wvalue jsonOut;

    for (const auto &&_item : _Users)
    {
      crow::json::wvalue element;

      element[_Index] = _item.index;
      element[_Username] = _item.pItem->username;

      for (int8_t i = 0; i < _item.pItem->availableTimePerDay.count; i++)
        element[_AvailableTimePerDay][i] = _item.pItem->availableTimePerDay[i];

      for (int8_t i = 0; i < _item.pItem->completedTasksForCurrentDay.count; i++)
        element[_CompletedTasks][i] = _item.pItem->completedTasksForCurrentDay[i];

      jsonOut[idx] = std::move(element);
      idx++;
    }
    
    stringOut = jsonOut.dump();
  }

  if (stringOut == "null")
    print_error_line("Failed to write users pool to file. File content is 'null'.");

  lsWriteFile(_FileNameUsers, stringOut.c_str(), stringOut.size());
  if (LS_FAILED(lsWriteFile("userspool.json", stringOut.c_str(), stringOut.size())))
    print_error_line("Failed to write users pool to file."); 
}

void writeEventsPoolToFile()
{
  std::string outString;

  // Mutex Lock
  {
    crow::json::wvalue jsonOut;
    std::scoped_lock lock(_ThreadLock);
    int idx = 0;

    for (const auto &&_item : _Events)
    {
      crow::json::wvalue element;

      element[_Index] = _item.index;
      element[_Name] = _item.pItem->name;
      element[_DurationTimeSpan] = _item.pItem->durationTimeSpan;

      for (int8_t i = 0; i < _item.pItem->userIds.count; i++)
        element[_UsersIds][i] = _item.pItem->userIds[i];

      element[_Weight] = _item.pItem->weight;
      element[_WeightFactor] = _item.pItem->weightGrowthFactor;
      element[_PossibleExecutionDays] = _item.pItem->possibleExecutionDays;
      element[_RepetitionTimeSpan] = _item.pItem->repetitionTimeSpan;
      element[_LastCompletedTime] = _item.pItem->lastCompletedTime;
      element[_LastModifiedTime] = _item.pItem->lastModifiedTime;
      element[_CreationTime] = _item.pItem->creationTime;

      jsonOut[idx] = std::move(element);
      idx++;
    }

    outString = jsonOut.dump();
  }

  if (outString == "null")
    print_error_line("Failed to write events pool to file. File content is 'null'.");

  lsWriteFile(_FileNameEvents, outString.c_str(), outString.size());
  if (LS_FAILED(lsWriteFile("eventspool.json", outString.c_str(), outString.size())))
    print_error_line("Failed to write events pool to file.");
}
  
//////////////////////////////////////////////////////////////////////////

void deserializeUsersPool()
{
  char *fileContents = nullptr;
  size_t fileSize;

  if (LS_FAILED(lsReadFile(_FileNameUsers, &fileContents, &fileSize)))
    print_error_line("Failed to read file", _FileNameUsers);

  const auto &jsonRoot = crow::json::load(fileContents);

  for (const auto &_item : jsonRoot)
  {
    size_t index = _item[_Index].i();
    user usr;

    std::string username = _item[_Username].s();
    if (username.length() == 0)
      print_error_line("Filecontent of ", _FileNameUsers, " invalid: Lenght of username is 0.");

    strncpy(usr.username, username.c_str(), LS_ARRAYSIZE(usr.username));

    for (const auto _t : _item[_AvailableTimePerDay])
    {
      int64_t timeValue = _t.i();

      if (timeValue < 0)
        print_error_line("Filecontent of ", _FileNameUsers, " invalid: availableTimePerDay < 0 (", timeValue, ")");

      local_list_add(&usr.availableTimePerDay, (time_span_t)timeValue);
    }

    if (_item.has(_CompletedTasks))
    {
      for (const auto &_i : _item[_CompletedTasks])
      {
        int64_t eventId = _i.i();

        if (eventId < 0)
          print_error_line("Filecontent of ", _FileNameUsers, " invalid: eventId of completedTasksForCurrentDay < 0 (", eventId, ")");

        local_list_add(&usr.completedTasksForCurrentDay, (size_t)eventId);
      }
    }

    pool_add(&_Users, &usr, &index);
  }

//epilogue:
  lsFreePtr(&fileContents);
}

void deserialzieEventsPool()
{
  char *fileContents = nullptr;
  size_t fileSize;

  if (LS_FAILED(lsReadFile(_FileNameUsers, &fileContents, &fileSize)))
    print_error_line("Failed to read file", _FileNameEvents);

  const auto &jsonRoot = crow::json::load(fileContents);

  for (const auto &_item : jsonRoot)
  {
    size_t index = _item[_Index].i();
    event evnt;

    std::string name = _item[_Name].s();
    if (name.length() == 0)
      print_error_line("Filecontent of ", _FileNameEvents, " invalid: Lenght of event name is 0.");

    strncpy(evnt.name, name.c_str(), LS_ARRAYSIZE(evnt.name));
  }

//epilogue:
  lsFreePtr(&fileContents);
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
    local_list_add(&usr.availableTimePerDay, time_span_from_minutes(_item.i()));
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

  if (!body || !body.has("sessionId") || !body.has("name") || !body.has("duration") || !body.has("possibleExecutionDays") || !body.has("repetition") || !body.has("weight") || !body.has("weightFactor") || !body.has("userIds"))
    return crow::response(crow::status::BAD_REQUEST);

  const int32_t sessionId = (int32_t)body["sessionId"].i();
  const std::string &eventName = body["name"].s();
  const uint64_t duration = body["duration"].i();
  const uint64_t repetitionInDays = body["repetition"].i();
  const uint64_t weight = body["weight"].i();
  const uint64_t weightFactor = body["weightFactor"].i();
  local_list<bool, 7> executionDays;
  local_list<size_t, maxUsersPerEvent> userIds;

  size_t __unused;
  if (LS_FAILED(get_user_id_from_session_id(sessionId, &__unused)))
    return crow::response(crow::status::FORBIDDEN);
  
  for (const auto &b : body["possibleExecutionDays"])
    if (LS_FAILED(local_list_add(&executionDays, b.b())))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  for (const auto &i : body["userIds"])
  {
    lsAssert(i.i() >= 0);

    if (i.i() < 0)
      return crow::response(crow::status::BAD_REQUEST);;

    if (LS_FAILED(local_list_add(&userIds, (size_t)i.i())))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
  }

  event evnt;

  for (const auto &_item : userIds)
  {
    if (LS_FAILED(local_list_add(&evnt.userIds, _item)))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
  }

  if (eventName.length() == 0 || eventName.length() > LS_ARRAYSIZE(evnt.name))
    return crow::response(crow::status::BAD_REQUEST);

  strncpy(evnt.name, eventName.c_str(), LS_ARRAYSIZE(evnt.name));

  if (repetitionInDays < 0)
    return crow::response(crow::status::BAD_REQUEST);

  evnt.repetitionTimeSpan = time_span_from_days(repetitionInDays);

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

  if (isCreation)
  {
    evnt.creationTime = get_current_time();
    evnt.lastCompletedTime = 0;
    evnt.lastModifiedTime = 0;

    if (LS_FAILED(add_new_event(evnt)))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);
  }
  else
  {
    if (!body.has("id"))
      return crow::response(crow::status::BAD_REQUEST);

    size_t id = body["id"].i();

    event oldEvent;
    if (LS_FAILED(get_event(id, &oldEvent)))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);

    evnt.creationTime = oldEvent.creationTime;
    evnt.lastCompletedTime = oldEvent.lastCompletedTime;
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

  const int32_t sessionId = (int32_t)body["sessionId"].i();

  size_t userId;
  if (LS_FAILED(get_user_id_from_session_id(sessionId, &userId)))
    return crow::response(crow::status::FORBIDDEN);

  // TODO: This needs a flag for already completed tasks
  local_list<event_info, maxEventsPerUserPerDay> currentTasks;
  if (LS_FAILED(get_current_events_from_user_id(userId, &currentTasks)))
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

crow::response handle_event_search(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId") || !body.has("query"))
    return crow::response(crow::status::BAD_REQUEST);

  const std::string &query = body["query"].s();
  const int32_t sessionId = (int32_t)body["sessionId"].i();

  size_t __unused;
  if (LS_FAILED(get_user_id_from_session_id(sessionId, &__unused)))
    return crow::response(crow::status::FORBIDDEN);

  local_list<event_info, maxSearchResults> searchResults;

  if (LS_FAILED(search_events(query.c_str(), &searchResults)))
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

crow::response handle_user_search(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId") || !body.has("query"))
    return crow::response(crow::status::BAD_REQUEST);

  const int32_t sessionId = (int32_t)body["sessionId"].i();

  size_t __unused;
  if (LS_FAILED(get_user_id_from_session_id(sessionId, &__unused)))
    return crow::response(crow::status::FORBIDDEN);

  const std::string &query = body["query"].s();

  local_list<user_info, maxSearchResults> searchResults;
  if (LS_FAILED(search_users(query.c_str(), &searchResults)))
    return crow::response(crow::status::INTERNAL_SERVER_ERROR);

  crow::json::wvalue ret = crow::json::rvalue(crow::json::type::List);

  for (int8_t i = 0; i < searchResults.count; i++)
  {
    ret[i]["name"] = searchResults[i].name;
    ret[i]["id"] = searchResults[i].id;
  }

  return crow::response(crow::status::OK, ret);
}

crow::response handle_event_completed(const crow::request &req, const bool needsReschdule)
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
    return crow::response(crow::status::FORBIDDEN);

  if (LS_FAILED(add_completed_task(eventId, userId)))
    return crow::response(crow::status::BAD_REQUEST);

  if (needsReschdule)
    _ExplicitlyRequestsReschedule++;

  crow::json::wvalue ret;
  ret["success"] = true;

  return crow::response(crow::status::OK, ret);
}

crow::response handle_task_details(const crow::request &req)
{
  auto body = crow::json::load(req.body);

  if (!body || !body.has("sessionId") || !body.has("taskId"))
    return crow::response(crow::status::BAD_REQUEST);

  const size_t taskId = body["taskId"].i();
  const int32_t sessionId = (int32_t)body["sessionId"].i();

  size_t __unused;
  if (LS_FAILED(get_user_id_from_session_id(sessionId, &__unused)))
    return crow::response(crow::status::FORBIDDEN);

  event evnt;
  if (LS_FAILED(get_event(taskId, &evnt)))
    return crow::response(crow::status::BAD_REQUEST);

  crow::json::wvalue ret;

  ret["name"] = evnt.name;

  ret["duration"] = minutes_from_time_span(evnt.durationTimeSpan);

  for (int8_t i = 0; i < DaysPerWeek; i++)
    ret["executionDays"][i] = !!(evnt.possibleExecutionDays & (1 << i));

  ret["repetition"] = days_from_time_span(evnt.repetitionTimeSpan);
  ret["weight"] = evnt.weight;
  ret["weightFactor"] = evnt.weightGrowthFactor;

  ret["users"] = crow::json::rvalue(crow::json::type::List);

  for (int8_t i = 0; i < evnt.userIds.count; i++)
  {
    user_info info;

    if (LS_FAILED(get_user_info(evnt.userIds[i], &info)))
      return crow::response(crow::status::INTERNAL_SERVER_ERROR);

    ret["users"][i]["name"] = info.name;
    ret["users"][i]["id"] = info.id;
  }

  return crow::response(crow::status::OK, ret);
}
