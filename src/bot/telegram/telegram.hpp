#pragma once

#include "keleqram_bot.hpp"
#include "interfaces/interfaces.hpp"
#include "INIReader.h"

namespace kbot {
namespace keleqram {
namespace constants {
static const uint8_t     APP_NAME_LENGTH{6};
static const std::string USER{};
static const char*       DEFAULT_CONFIG_PATH{"config/config.ini"};
static const char*       REQUEST_PollStop   {"poll stop"};
static const char*       REQUEST_PollResult {"poll result"};
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
    ::keleqram::KeleqramBot::Poll();
}


void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}

enum class TGCommand
{
message   = 0x00,
poll      = 0x01,
poll_stop = 0x02
};

bool HandleEvent(BotRequest request)
{
  using namespace kbot::keleqram::constants;
  static const uint32_t msg_cmd   = static_cast<uint32_t>(TGCommand::message);
  static const uint32_t poll_cmd  = static_cast<uint32_t>(TGCommand::poll);
  static const uint32_t poll_stop = static_cast<uint32_t>(TGCommand::poll_stop);

  auto GetURLS = [](const std::vector<std::string>& v)
  {
    bool found_image = false;
    std::vector<std::string> urls;
    for (const auto& url : v)
    {
      auto mime = ::keleqram::GetMimeType(url);
      if (mime.IsPhoto())
      {
        if (!found_image)
        {
          urls.push_back(url);
          found_image = true;
        }
      }
      else
        urls.push_back(url);
    }
    return urls;
  };

  auto GetPollArgs = [](const std::vector<std::string>& v) { return std::vector<std::string>{v.begin() + 1, v.end()}; };
        bool  error = false;
  const auto  event = request.event;
  const auto  post  = request.data;
  const auto  urls  = request.urls;
  const auto  cmd   = request.cmd;
  const auto  args  = kbot::keleqram::GetArgs(request.args);
  const auto  dest  = (args.empty()) ? "" : args.front();
  std::string err_msg;

  if (event == "livestream active" || event == "platform:repost" || event == "telegram:messages")
  {
    try
    {
      switch (cmd)
      {
        case (msg_cmd):
          for (const auto& url : GetURLS(request.urls))
            KeleqramBot::SendMedia(url, dest);
          KeleqramBot::SendMessage(post, dest);
        break;
        case (poll_stop):
          m_send_event_fn(CreateRequest(
            REQUEST_PollResult,
            KeleqramBot::StopPoll(dest, GetPollArgs(args).front()),
            request));
        break;
        case (poll_cmd):
          m_send_event_fn(CreateRequest(
            REQUEST_PollStop,
            KeleqramBot::SendPoll(post, dest, GetPollArgs(args)),
            request));
        break;
      }
    }
    catch (const std::exception& e)
    {
      err_msg += "Exception caught handling " + request.event + ": " + e.what();
      log(err_msg);
      error = true;
    }
  }

  m_send_event_fn((error) ? CreateErrorEvent(err_msg, request) :
                            CreateSuccessEvent(request));

  return !error;
}

virtual std::unique_ptr<API> GetAPI(std::string name) override
{
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
