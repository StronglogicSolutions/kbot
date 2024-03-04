#pragma once

#include "keleqram_bot.hpp"
#include "interfaces/interfaces.hpp"
#include "INIReader.h"
#include <logger.hpp>

namespace kiq::kbot {
namespace keleqram {
namespace constants {
static const uint8_t     APP_NAME_LENGTH{6};
static const uint8_t     MAX_FAILURE_LIMIT{5};
static const std::string USER{};
static const char*       DEFAULT_CONFIG_PATH{"config/config.ini"};
static const char*       REQUEST_PollStop   {"poll stop"};
static const char*       REQUEST_PollResult {"poll result"};
static const char*       REQUEST_Rooms      {"process rooms"};
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

using namespace kiq::log;

class TelegramBot : public kbot::Worker,
                    public kbot::Bot,
                    public ::keleqram::KeleqramBot
{
public:
TelegramBot()
: kbot::Bot{keleqram::constants::USER},
  ::keleqram::KeleqramBot{keleqram::GetToken()},
  m_retries(keleqram::constants::MAX_FAILURE_LIMIT)
{}

TelegramBot& operator=(const TelegramBot& bot)
{
  m_retries = bot.m_retries;
  return *this;
}

virtual void Init(bool flood_protect) override
{
  return;
}

virtual void loop() override
{
  Worker::m_is_running = true;
  static int32_t i = 0;
  try
  {
    while (m_is_running)
      ::keleqram::KeleqramBot::Poll();
    klog().w("Telegram worker is no longer running");
  }
  catch(const std::exception& e)
  {
    klog().e("Exception caught while polling for updates: {}", e.what());

    if (!--m_retries)
    {
      m_send_event_fn(BotRequest{Platform::telegram, kbot::RESTART_EVENT});
      m_retries = keleqram::constants::MAX_FAILURE_LIMIT;
    }
    else
     loop();
  }
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

bool HandleEvent(const BotRequest& request)
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
      auto mime = kutils::GetMimeType(url);
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
  auto GetDest     = [](const std::vector<std::string>& v) -> std::string
  {
    auto FindEnd = [](const std::string& s) { for (size_t i = 1; i < s.size(); i++) if (!std::isdigit(s[i])) return i; return s.size(); };

    if ((v.size()) && (v.front().size() > 2) && isdigit(v.front().at(1)))
    {
      auto s = v.front();
      auto i = FindEnd(s);
      return s.substr(0, i);
    }
    return "";
  };
  auto GetPollArgs = [](const std::vector<std::string>& v) { return std::vector<std::string>{v.begin() + 1, v.end()};                   };
        bool  error = false;
        bool  reply = true;
  const auto  event = request.event;
  const auto  data  = request.data;
  const auto  urls  = request.urls;
  const auto  cmd   = request.cmd;
  const auto  args  = kbot::keleqram::GetArgs(request.args);
  const auto  dest  = GetDest(args);
  std::string err_msg;
  try
  {
    if (event == "livestream active" || event == "platform:repost" || event == "telegram:messages")
    {
      switch (cmd)
      {
        case (msg_cmd):
          for (const auto& url : GetURLS(request.urls))
            KeleqramBot::SendMedia(url, dest);
          KeleqramBot::SendMessage(data, dest);
        break;
        case (poll_stop):
          m_send_event_fn(CreateRequest(
            REQUEST_PollResult,
            KeleqramBot::StopPoll(dest, GetPollArgs(args).front()),
            request));
        break;
        case (poll_cmd):
        {
          klog().i("Sending Poll to Telegram");
          m_send_event_fn(CreateRequest(
            REQUEST_PollStop,
            KeleqramBot::SendPoll(data, dest, GetPollArgs(args)),
            request));
          klog().w("IPC Response should be request to schedule PollStop");
        }
        break;
      }
    }
    else
    if (event == "telegram:delete")
    {
      reply = false;
      KeleqramBot::DeleteMessages(dest, ::keleqram::DeleteAction{"/delete last " + args.at(1)});
    }
    else
    if (event == "telegram:rooms")
    {
      reply = false;
      m_send_event_fn(CreateInfo(KeleqramBot::GetRooms(), "rooms", request));
    }
    else
    if (event == "telegram:restart")
    {
      reply = false;
      m_send_event_fn(BotRequest{Platform::telegram, kbot::RESTART_EVENT});
    }
  }
  catch (const std::exception& e)
  {
    err_msg += "Exception caught handling " + request.event + ": " + e.what();
    klog().e(err_msg);
    error = true;
  }
  if (reply)
    m_send_event_fn((error) ? CreateErrorEvent(err_msg, request) : CreateSuccessEvent(request));

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
unsigned int   m_retries;
};
} // namespace kiq::kbot
