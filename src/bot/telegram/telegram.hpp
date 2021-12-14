#pragma once

#include "keleqram_bot.hpp"
#include "interfaces/interfaces.hpp"
#include "INIReader.h"

namespace kbot {
namespace keleqram {
namespace constants {
static const std::string USER{};
static const uint8_t     APP_NAME_LENGTH{12};
static const std::string DEFAULT_CONFIG_PATH{"config/config.ini"};
} // namespace constants

static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::APP_NAME_LENGTH  + 1));
}

static const INIReader GetConfig()
{
  return INIReader{get_executable_cwd() + "/../" + constants::DEFAULT_CONFIG_PATH};
}

static std::string GetToken()
{
  return GetConfig().GetString("telegram_bot", "token", "");
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
} // ns keleqram

class TelegramBot : public kbot::Worker,
                    public kbot::Bot,
                    public ::keleqram::KeleqramBot
{
public:
TelegramBot()
: kbot::Bot{keleqram::constants::USER},
  ::keleqram::KeleqramBot{keleqram::GetToken()}
{}

virtual void Init() override
{
  return;
}

virtual void loop() override
{
  while (m_is_running)
  {
    ::keleqram::KeleqramBot::Poll();
    ::keleqram::log("TelegramBot alive");
  }
}


void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}

enum class TGCommand
{
message = 0x00,
poll    = 0x01
};

bool HandleEvent(BotRequest request)
{
  static const uint32_t msg_cmd  = 0x00;
  static const uint32_t poll_cmd = 0x01;

  auto GetPollArgs = [](const std::vector<std::string>& v) { return std::vector<std::string>{v.begin() + 1, v.end()}; };
        bool  error = false;
  const auto  event = request.event;
  const auto  post  = request.data;
  const auto  urls  = request.urls;
  const auto  cmd   = request.cmd;
  const auto  args  = kbot::keleqram::GetArgs(request.args);
  const auto  dest  = args.front();
  std::string error_message;

  if (event == "livestream active" || event == "platform:repost" || event == "telegram:messages")
  {
    try
    {
      switch (cmd)
      {
        case (msg_cmd):
          for (const auto& url : request.urls)
            ::keleqram::KeleqramBot::SendMedia(url, dest);
          ::keleqram::KeleqramBot::SendMessage(post, dest);
        break;
        case (poll_cmd):
          ::keleqram::KeleqramBot::SendPoll(post, dest, GetPollArgs(args));
        break;
      }
    }
    catch (const std::exception& e)
    {
      error_message = "Exception caught:", e.what();
      log(error_message);
      error = true;
    }
  }

  m_send_event_fn((error) ? CreateErrorEvent(error_message, request) :
                            CreateSuccessEvent(request));

  return !error;
}

virtual std::unique_ptr<API> GetAPI(std::string name) override
{
  // TODO: Add other APIs
  return nullptr;
}

virtual bool IsRunning() override
{
  return m_is_running;
}

virtual void Start() override
{
  if (!m_is_running)
    Worker::start();
}

virtual void Shutdown() override
{
  Worker::stop();
}

private:
BrokerCallback m_send_event_fn;

};
} // namespace kbot
