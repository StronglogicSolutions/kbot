#pragma once

#include "kstodon/config.hpp"
#include "interfaces/interfaces.hpp"

/*
using GenerateFunction = Status(*)();
using ReplyFunction    = Status(*)(Status status);
*/

namespace kbot {
namespace constants {
const std::string USERNAME{
  kstodon::GetConfigReader().GetString("kstodon", "user", "koreannews")
};
} // namespace constants
/**
 * @brief
 *
 * @return Status
 */
kstodon::Status GenerateStatusMessage()
{
  return kstodon::Status{"Hello from KSTYLEYO!"};
}

kstodon::Status ReplyToMastodonMessage(kstodon::Status status)
{
  return kstodon::Status{"Nice to hear from you."};
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
    std::cout << "MastodonBot alive" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }
}


void SendPublicMessage(const std::string& message, const std::vector<std::string>& file_urls)
{
  PostStatus(kstodon::Status{message}, file_urls);

  m_send_event_fn(BotEvent{
    .platform = Platform::mastodon,
    .name = "platform:post",
    .data = message,
    .urls = file_urls,
    .id = ""
  });
}

void SendPrivateMessage(const std::string& message, const std::string& user_id)
{
  kstodon::Status status{message};
  status.visibility = kstodon::constants::StatusVisibility::DIRECT;
  status.replying_to_id = user_id;
  PostStatus(status);
}

void SetCallback(BrokerCallback cb_fn) {
  m_send_event_fn = cb_fn;
}

bool HandleEvent(BotEvent event) {
  std::stringstream ss{};

  if (event.name == "comments find")
  {
    for (const kstodon::Status& status : FindComments())
      ss << status;

    m_send_event_fn(BotEvent{
      .platform = Platform::mastodon,
      .name     = "comment",
      .data     = ss.str()
    });
  }
  else
  if (event.name == "livestream active")
  {
    SendPublicMessage(event.data, event.urls);
  }

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
