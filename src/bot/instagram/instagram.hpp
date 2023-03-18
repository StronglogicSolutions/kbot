#pragma once

#include "interfaces/interfaces.hpp"

namespace kbot {
namespace kgram {
namespace constants {
static const uint8_t     APP_NAME_LENGTH{6};
static const std::string USER{};
} // namespace constants

static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::APP_NAME_LENGTH + 1));
}

} // ns kgram

class InstagramBot : public kbot::Worker,
                     public kbot::Bot
{
public:
InstagramBot()
: kbot::Bot{kgram::constants::USER}
{}

virtual void Init() override
{
  return;
}

virtual void loop() override
{
  Worker::m_is_running = true;
  // TODO:
  // Get Messages
  // Send requests
}


void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}

bool HandleEvent(const BotRequest& request)
{
        bool  error = false;
        bool  reply = true;
  const auto  event = request.event;
  std::string err_msg;
  try
  {
    if (event == "livestream active" || event == "platform:repost" || event == "instagram:messages")
    {
      (void)(0);
    }
  }
  catch (const std::exception& e)
  {
    err_msg += "Exception caught handling " + request.event + ": " + e.what();
    log(err_msg);
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
};
} // namespace kgram
