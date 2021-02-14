#pragma once

#include "kscord/kscord.hpp"
#include "interfaces/interfaces.hpp"


namespace kbot {
namespace constants {
const std::string USER{"stronglogicsolutions"};
} // namespace constants


class DiscordBot : public  kbot::Worker,
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
  {
    std::cout << "DiscordBot alive" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }
}


void SendPublicMessage(const std::string& message)
{
  (void)(0);
}

void SendPrivateMessage(const std::string& message, const std::string& user_id)
{
  (void)(0);
}

void SetCallback(BrokerCallback cb_fn) {
  m_send_event_fn = cb_fn;
}

bool HandleEvent(BotEvent event) {
  (void)(0);
  return false;
}

virtual std::unique_ptr<API> GetAPI(std::string name) override
{
  // TODO: Determine if we really need this type of interface
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
