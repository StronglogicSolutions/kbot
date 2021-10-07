#pragma once

#include "keleqram_bot.hpp"
#include "interfaces/interfaces.hpp"
#include "INIReader.h"

namespace kbot {
namespace constants {
static const std::string USER{""};
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

class TelegramBot : public kbot::Worker,
                    public kbot::Bot,
                    public keleqram::KeleqramBot
{
public:
TelegramBot()
: kbot::Bot{constants::USER},
  keleqram::KeleqramBot{GetToken()}
{}

virtual void Init() override
{
  return;
}

virtual void loop() override
{
  while (m_is_running)
    keleqram::KeleqramBot::Poll();
}


void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}

bool HandleEvent(BotRequest request)
{
        bool error{false};
  const auto event = request.event;
  const auto post  = request.data;
  const auto urls  = request.urls;
  std::string error_message;

  if (event == "livestream active" || event == "platform:repost")
  {
    try
    {
      for (const auto& url : request.urls)
        keleqram::KeleqramBot::SendPhoto(url);
      keleqram::KeleqramBot::SendMessage(post);
    }
    catch (const std::exception& e)
    {
      error_message = "Exception caught:", e.what();
      log(error_message);
      error = true;
    }
  }

  if (error)
  {
    log(error_message);
    m_send_event_fn(CreateErrorEvent(error_message, request));
  }
  else
    m_send_event_fn(CreateSuccessEvent(request));

  return (!error);
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
