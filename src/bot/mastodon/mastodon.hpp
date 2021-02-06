#pragma once

#include "kstodon/config.hpp"
#include "interfaces/interfaces.hpp"

/*
using GenerateFunction = Status(*)();
using ReplyFunction    = Status(*)(Status status);
*/

namespace kbot {
namespace constants {
const std::string USERNAME{"koreannews"};
} // namespace constants
/**
 * @brief
 *
 * @return Status
 */
Status GenerateStatusMessage()
{
  return Status{"Hello from KSTYLEYO!"};
}

Status ReplyToMastodonMessage(Status status)
{
  return Status{"Nice to hear from you."};
}

class MastodonBot : public kbot::Worker,
                    public kbot::Bot,
                    public kstodon::Bot
{
public:
MastodonBot()
: kbot::Bot{constants::USERNAME},
  kstodon::Bot{constants::USERNAME, &GenerateStatusMessage, &ReplyToMastodonMessage}
{}

virtual void Init() override
{
  return;
}

virtual void loop() override
{
  while (m_is_running)
  {
    std::cout << "Hello" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}


void SendPublicMessage(const std::string& message)
{
  PostStatus(Status{message});
}

void SendPrivateMessage(const std::string& message, const std::string& user_id)
{
  Status status{message};
  status.visibility = ::constants::StatusVisibility::DIRECT;
  status.replying_to_id = user_id;
  PostStatus(status);
}

void SetCallback(BrokerCallback cb_fn) {
  m_send_event_fn = cb_fn;
}

bool HandleEvent(BotEvent event) {
  std::stringstream ss{};

  if (event.name == "find comments")
  {
    for (const Status& status : FindComments())
      ss << status;
  }

  std::string result = ss.str();

  m_send_event_fn(BotEvent{
    .platform = Platform::mastodon,
    .name     = "comment",
    .data     = result
  });

  return true;
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
