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
  bool error{false};

  if (event.name == "comments find")
  {
    std::stringstream ss{};
    for (const kstodon::Status& status : FindComments())
      ss << status;

    std::string comment_string = ss.str();
    if (!comment_string.empty())
      m_send_event_fn(BotEvent{
        .platform = Platform::mastodon,
        .name     = "comment",
        .data     = comment_string
      });

    return true;
  }
  else
  if (event.name == "livestream active")
  {
    if (!PostStatus(kstodon::Status{event.data}, event.urls))
      error = true;
  }
  else
  if (event.name == "platform:repost")
  {
    kstodon::Status status{event.data};
    status.visibility = kstodon::constants::StatusVisibility::UNLISTED;
    std::vector<std::string> urls = (event.urls.empty()) ? std::vector<std::string>{} : std::vector<std::string>{event.urls.front()};
    if (!PostStatus(status, urls))
      error = true;
  }

  if (error)
  {
    std::string error_message{"Failed to handle " + event.name + " event"};
    kbot::log(error_message);
    m_send_event_fn(CreateErrorEvent(error_message, event));
  }
  else
    m_send_event_fn(CreateSuccessEvent(event));

  return (!error);
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
