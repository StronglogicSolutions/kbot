#pragma once

#include "kscord/kscord.hpp"
#include "interfaces/interfaces.hpp"

namespace kbot {
namespace constants {
const std::string USER{""};
} // namespace constants

class DiscordBot : public kbot::Worker,
                   public kbot::Bot,
                   public kscord::Client
{
public:
DiscordBot()
: kbot::Bot{constants::USER},
  kscord::Client{}
{}

virtual void Init() override
{
  return;
}

virtual void loop() override
{
  while (m_is_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}


void SendPublicMessage(const std::string& message, const std::vector<std::string> urls = {})
{

}

void SendPrivateMessage(const std::string& message, const std::string& user_id)
{
  (void)(0);
}

void SetCallback(BrokerCallback cb_fn) {
  m_send_event_fn = cb_fn;
}

bool HandleEvent(BotRequest request) {
  bool error{false};
  const auto event = request.event;

  if (!request.username.empty()                          &&
       kscord::Client::GetUsername() != request.username &&
      !kscord::Client::SetUser(request.username))
      error = true;
  else
  {
    if (event == "livestream active" ||
        event == "discord:messages"    )
    {
      error = !kscord::Client::PostMessage(request.data);
    }
    else
    if (event == "platform:repost")
    {
      error = !kscord::Client::PostMessage(request.data, request.urls);
    }
  }

  if (error)
  {
    std::string error_message{"Failed to handle " + request.event + "\nLast error: " + kscord::Client::GetLastError()};
    kbot::log(error_message);
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
