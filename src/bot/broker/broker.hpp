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
using EventQueue    = std::deque<BotRequest>;
using u_ipc_msg_ptr = ipc_message::u_ipc_msg_ptr;

inline std::vector<std::string> GetArgs(std::string s) {
  using json = nlohmann::json;
  json d = json::parse(s, nullptr, false);

  if (!d.is_null() && d.contains("args")) {
    return d["args"].get<std::vector<std::string>>();
  }
  return {};
}

inline const BotRequest CreatePlatformEvent(platform_message* message)
{
  return BotRequest{
    .platform = get_platform(message->platform()),
    .event    = "platform:repost",
    .username = UnescapeQuotes(message->user()),
    .data     = UnescapeQuotes(message->content()),
    .urls     = BotRequest::urls_from_string(message->urls()),
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
const uint8_t IPC_USER_INDEX   {0x02};
const uint8_t IPC_PARAM_NUMBER{0x03};

bool ValidIPCArguments(const std::vector<std::string>& arguments)
{
  return arguments.size() >= IPC_PARAM_NUMBER;
}

void ProcessMessage(u_ipc_msg_ptr message) {

  if (message->type() == ::constants::IPC_KIQ_MESSAGE)
  {
    kiq_message* kiq_msg = static_cast<kiq_message*>(message.get());
    auto         args    = GetArgs(kiq_msg->payload());

    if (ValidIPCArguments(args));
    {
      const auto& command = args.at(IPC_COMMAND_INDEX);
      const auto& payload = args.at(IPC_PAYLOAD_INDEX);
      const auto& user    = args.at(IPC_USER_INDEX);
      Platform    platform{};

      if (command == "youtube:livestream")
        platform = Platform::youtube;
      else
      if (command == "mastodon:comments")
        platform = Platform::mastodon;
      else
      if (command == "discord:messages")
        platform = Platform::discord;

      SendEvent(BotRequest{platform, command, user, payload});
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
static bool ProcessEvent(BotRequest event)
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
void enqueue(BotRequest event)
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

  while (Worker::m_is_running)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
      m_condition.wait(lock,
        [this]()
        {
          return (YTBot().IsRunning() || MDBot().IsRunning());
        }
      );
    m_condition.notify_one();

    if (!m_queue.empty())
    {
      BotRequest  request = m_queue.front();
      const auto& event   = request.event;
      if (event == "platform:post")                          // ALL Platforms
      {
        m_outbound_queue.emplace_back(
          std::make_unique<platform_message>(
            get_platform_name(request.platform),
            request.id,
            request.username,
            request.data,
            request.url_string(),
            SHOULD_REPOST
          )
        );
      }
      else
      if (event == "livestream inactive")
          kbot::log("YouTube bot returned no livestreams");
      else
      if (event == "comment")
        kbot::log(get_platform_name(request.platform) + " bot has new comments: " + request.data);
      else
      if (event == "message")
        kbot::log(get_platform_name(request.platform) + " bot has new messages: " + request.data);
      else
      if (event == "bot:success")
      {
        kbot::log(get_platform_name(request.platform) + " successfully handled " + request.previous_event);

        if (request.previous_event == "platform:repost") // ALL Platforms
        {
          m_outbound_queue.emplace_back(
            std::make_unique<platform_message>(
              get_platform_name(request.platform),
              request.id,
              request.username,
              request.data,
              request.url_string(),
              SHOULD_NOT_REPOST
            )
          );
        }
      }
      else
      if (event == "bot:error")
      {
        kbot::log(get_platform_name(request.platform) + " failed to handle " + request.previous_event);
        const std::string error_message = request.data;
        const std::string id            = request.id;
        m_outbound_queue.emplace_back(
          std::make_unique<platform_error>(
            get_platform_name(request.platform),
            request.id,
            request.username,
            request.data
          )
        );
      }
      else
        SendEvent(request);

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
void SendEvent(const BotRequest& event)
{
  try
  {
    if (event.platform == Platform::discord)
      DCBot().HandleEvent(event);
    else
    if (event.platform == Platform::mastodon)
      MDBot().HandleEvent(event);
    else
    if (event.platform == Platform::youtube)
      YTBot().HandleEvent(event);
    else
      kbot::log("Unable to send event for unknown platform");
  }
  catch (const std::exception& e)
  {
    kbot::log("Exception caught");
    kbot::log(e.what());
  }
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