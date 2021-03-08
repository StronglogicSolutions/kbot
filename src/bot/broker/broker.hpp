#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

#include "bot/mastodon/mastodon.hpp"
#include "bot/youtube/youtube.hpp"
#include "bot/discord/discord.hpp"
#include "ipc.hpp"

namespace kbot {
using u_bot_ptr     = std::unique_ptr<Bot>;
using BotPool       = std::vector<u_bot_ptr>;
using EventQueue    = std::deque<BotEvent>;
using u_ipc_msg_ptr = ipc_message::u_ipc_msg_ptr;

inline std::vector<std::string> GetArgs(std::string s) {
  using json = nlohmann::json;
  json d = json::parse(s, nullptr, false);

  if (!d.is_null() && d.contains("args")) {
    return d["args"].get<std::vector<std::string>>();
  }
  return {};
}

inline const BotEvent CreatePlatformEvent(platform_message* message)
{
  return BotEvent{
    .platform = get_platform(message->name()),
    .name     = "platform:repost",
    .data     = UnescapeQuotes(message->content()),
    .urls     = BotEvent::urls_from_string(message->urls()),
    .id       = message->id()
  };
}

namespace constants {
const uint8_t YOUTUBE_BOT_INDEX {0x00};
const uint8_t MASTODON_BOT_INDEX{0x01};
const uint8_t DISCORD_BOT_INDEX{0x02};
} // namespace constants

Broker* g_broker;

class Broker : public Worker
{
public:
Broker()
{
  u_bot_ptr u_yt_bot_ptr{new kbot::YouTubeBot{}};
  u_bot_ptr u_md_bot_ptr{new kbot::MastodonBot{}};
  u_bot_ptr u_dc_bot_ptr{new kbot::DiscordBot{}};
  g_broker = this;

  m_pool.emplace_back(std::move(u_yt_bot_ptr));
  m_pool.emplace_back(std::move(u_md_bot_ptr));
  m_pool.emplace_back(std::move(u_dc_bot_ptr));

  YTBot().SetCallback(&ProcessEvent);
  MDBot().SetCallback(&ProcessEvent);
  DCBot().SetCallback(&ProcessEvent);
  YTBot().Init();
  MDBot().Init();
  DCBot().Init();

}

const uint8_t IPC_COMMAND_INDEX{0x00};
const uint8_t IPC_PAYLOAD_INDEX{0x01};
const uint8_t IPC_PARAM_NUMBER{0x02};

bool ValidIPCArguments(const std::vector<std::string>& arguments)
{
  return arguments.size() >= IPC_PARAM_NUMBER;
}

void ProcessMessage(u_ipc_msg_ptr message) {

  if (message->type() == ::constants::IPC_KIQ_MESSAGE)
  {
    kiq_message* kiq_msg = static_cast<kiq_message*>(message.get());
    const auto args = GetArgs(kiq_msg->payload());

    if (ValidIPCArguments(args));
    {
      const auto& command = args.at(IPC_COMMAND_INDEX);
      const auto& payload = args.at(IPC_PAYLOAD_INDEX);

      if (command == "youtube:livestream")
        SendEvent(BotEvent{.platform = Platform::youtube, .name = command, .data = payload});
      else
      if (command == "mastodon:comments")
        SendEvent(BotEvent{.platform = Platform::mastodon, .name = "comments:find", .data = payload});
      else
      if (command == "discord:messages")
        SendEvent(BotEvent{.platform = Platform::discord, .name = command, .data = payload});
    }
  }
  else
  if (message->type() == ::constants::IPC_PLATFORM_TYPE)
  {
    SendEvent(CreatePlatformEvent(static_cast<platform_message*>(message.get())));
  }
}

/**
 * @brief
 *
 * @param event
 * @return true
 * @return false
 */
static bool ProcessEvent(BotEvent event)
{
  if (g_broker != nullptr)
  {
    g_broker->enqueue(event);
    return true;
  }
  return false;
}

/**
 * @brief
 *
 * @param event
 */
void enqueue(BotEvent event)
{
  m_queue.emplace_back(event);
}

/**
 * @brief
 *
 */
void run()
{
  Worker::start();
}

/**
 * @brief
 *
 */
virtual void loop() override
{
  YTBot().Start();
  MDBot().Start();
  DCBot().Start();

  std::unique_lock<std::mutex> lock(m_mutex);

  while (Worker::m_is_running)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
      m_condition.wait(lock,
        [this]()
        {
          return (YTBot().IsRunning() || MDBot().IsRunning());
        }
      );
    lock.unlock();
    m_condition.notify_one();

    if (!m_queue.empty())
    {
      BotEvent event = m_queue.front();
      if (event.name == "platform:post")                            // ALL Platforms
      {
        m_outbound_queue.emplace_back(
          std::make_unique<platform_message>(
            get_platform_name(event.platform),
            event.id,
            event.data,
            event.url_string(),
            SHOULD_REPOST
          )
        );
      }
      else
      if (event.name == "livestream inactive")
          std::cout << "YouTube bot returned no livestreams" << std::endl;
      else
      if (event.name == "comment")
        std::cout << get_platform_name(event.platform) << " bot has new comments: " << event.data << std::endl;
      else
      if (event.name == "message")
        std::cout << get_platform_name(event.platform) << " bot has new messages: " << event.data << std::endl;
      else
      if (event.name == "bot:success")
      {
        kbot::log(get_platform_name(event.platform) + " successfully handled " + event.previous_name);

        if (event.previous_name == "platform:repost")                            // ALL Platforms
        {
          m_outbound_queue.emplace_back(
            std::make_unique<platform_message>(
              get_platform_name(event.platform),
              event.id,
              event.data,
              event.url_string(),
              SHOULD_NOT_REPOST
            )
          );
        }
      }
      else
      if (event.name == "bot:error")
      {
        kbot::log(get_platform_name(event.platform) + " failed to handle " + event.previous_name);
        const std::string error_message = event.data;
        const std::string id            = event.id;
        // TODO: Create IPC error message and send
        return;
      }
      else
        SendEvent(event);

      m_queue.pop_front();
    }

    m_condition.wait_for(lock, std::chrono::milliseconds(300));
  }
}

/**
 * SendEvent
 *
 * @param [in] {BotEvent}
 */
void SendEvent(const BotEvent& event)
{
  if (event.platform == Platform::discord)
    DCBot().HandleEvent(event);
  else
  if (event.platform == Platform::mastodon)
    MDBot().HandleEvent(event);
  else
  if (event.platform == Platform::youtube)
  {
    YTBot().HandleEvent(event);
  }
  else
    kbot::log("Unable to send event for unknown platform");
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool Shutdown()
{
  try
  {
    Bot& youtube_bot  = YTBot();
    Bot& mastodon_bot = MDBot();
    Bot& discord_bot  = DCBot();

    youtube_bot.Shutdown();
    mastodon_bot.Shutdown();
    discord_bot.Shutdown();

    while (youtube_bot.IsRunning() || mastodon_bot.IsRunning() || discord_bot.IsRunning())
    ;

    Worker::stop();

    return true;
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';

    return false;
  }
}

const bool Poll() const
{
  return !m_outbound_queue.empty();
}

u_ipc_msg_ptr DeQueue()
{
  u_ipc_msg_ptr message = std::move(m_outbound_queue.front());
  m_outbound_queue.pop_front();
  return std::move(message);
}

private:

Bot& YTBot()
{
  return *m_pool.at(constants::YOUTUBE_BOT_INDEX).get();
}

Bot& MDBot()
{
  return *m_pool.at(constants::MASTODON_BOT_INDEX).get();
}

Bot& DCBot()
{
  return *m_pool.at(constants::DISCORD_BOT_INDEX).get();
}

BotPool                    m_pool;
EventQueue                 m_queue;
std::deque<u_ipc_msg_ptr>  m_outbound_queue;
bool                       m_bots_active;
std::mutex                 m_mutex;
std::condition_variable    m_condition;
};

} // namespace kbot