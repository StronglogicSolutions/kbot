#pragma once

#include "interfaces/interfaces.hpp"

namespace kbot {
namespace constants {
const std::string USER{""};
} // namespace constants

class BlogBot : public kbot::Worker,
                   public kbot::Bot
{
public:
BlogBot()
: kbot::Bot{constants::USER}
{}

virtual void Init() override
{
  return;
}

virtual void loop() override
{
  while (m_is_running)
  {
    std::cout << "BlogBot alive" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }
}



virtual void SetCallback(BrokerCallback cb_fn) override
{
  m_send_event_fn = cb_fn;
}


virtual bool HandleEvent(BotRequest request) override
{
  const auto CreateBlogPost = [&](const std::string text, const std::vector<std::string> media_urls) -> bool
  {
    return false;
  };

  const auto event = request.event;

  (request.data); // text
  (request.urls); // media

  /**
   * TODO: HtmlBuilder?
   *
   */

  const bool error = CreateBlogPost(request.data, request.urls);
  if (error)
  {
    std::string error_message{"Failed to handle " + request.event};
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
