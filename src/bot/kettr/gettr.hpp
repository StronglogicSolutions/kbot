#pragma once

#include "kettr.hpp"
#include "interfaces/interfaces.hpp"
#include "INIReader.h"

namespace kbot {
namespace kettr {
namespace constants {
static const uint8_t     APP_NAME_LENGTH{6};
static const uint8_t     MAX_FAILURE_LIMIT{5};
static const std::string USER{};
static const char*       DEFAULT_CONFIG_PATH{"config/config.ini"};
} // namespace constants

static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::APP_NAME_LENGTH + 1));
}

static const INIReader GetConfig()
{
  return INIReader{get_executable_cwd() + "/../" + constants::DEFAULT_CONFIG_PATH};
}

static std::string GetUsername()
{
  return GetConfig().GetString("gettr_bot", "username", "");
}

static std::string GetPassword()
{
  return GetConfig().GetString("gettr_bot", "password", "");
}

static std::vector<std::string> GetArgs(const std::string& s)
{
  using json = nlohmann::json;
  json d = json::parse(s, nullptr, false);

  if (!d.is_null() && d.contains("args")) {
    return d["args"].get<std::vector<std::string>>();
  }
  return {};
}
} // ns kettr

class GettrBot : public kbot::Worker,
                 public kbot::Bot,
                 public kettr
{
public:
  GettrBot()
  : kbot::Bot{kbot::kettr::constants::USER},
    kettr{kbot::kettr::GetUsername(), kbot::kettr::GetPassword()}
  {}

GettrBot& operator=(const GettrBot& bot)
{
  m_retries = bot.m_retries;
  return *this;
}

virtual void Init() override
{
  if (kettr::login())
    log("GettrBot logged in");
  else
    log("GettrBot failed to login");
}

virtual void loop() override
{
  Worker::m_is_running = true;
  log("Gettr worker doesn't need to loop");
}
//--------------------------------------------------------
void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}
//--------------------------------------------------------
bool HandleEvent(const BotRequest& request)
{
  using namespace kbot::kettr::constants;

  auto post_event = [](const auto& e) { return (e == "livestream active" || e == "platform:repost" ||
                                                e == "gettr:messages"); };
        bool   error = false;
  const auto&  event = request.event;
  const auto&  data  = request.data;
  std::string  err_msg;

  try
  {
    if (post_event(event))
    {
      kettr::post(data);
      m_send_event_fn(CreateSuccessEvent(request));
    }
  }
  catch (const std::exception& e)
  {
    error    = true;
    err_msg += "Exception caught handling " + request.event + ": " + e.what();
    log(err_msg);
    m_send_event_fn(CreateErrorEvent(err_msg, request));
  }

  return !error;
}
//--------------------------------------------------------
virtual std::unique_ptr<API> GetAPI(std::string name) final
{
  return nullptr;
}
//--------------------------------------------------------
virtual bool IsRunning() final
{
  return m_is_running;
}
//--------------------------------------------------------
virtual void Start() final
{
  if (!m_is_running)
    Worker::start();
}

virtual void Shutdown() final
{
  Worker::stop();
}

private:
BrokerCallback m_send_event_fn;
unsigned int   m_retries;
};
} // namespace kbot



