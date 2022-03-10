#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

#include "bot/mastodon/mastodon.hpp"
#include "bot/youtube/youtube.hpp"
#include "bot/discord/discord.hpp"
#include "bot/telegram/telegram.hpp"
#include "bot/blog/blog.hpp"
#include "ipc.hpp"

#define OPENSSL_API_COMPAT 0x0908

namespace kbot {
using u_bot_ptr     = std::unique_ptr<Bot>;
using BotPool       = std::vector<u_bot_ptr>;
using EventQueue    = std::deque<BotRequest>;
using u_ipc_msg_ptr = ipc_message::u_ipc_msg_ptr;

static std::vector<std::string> GetArgs(std::string s) {
  using json = nlohmann::json;
  json d = json::parse(s, nullptr, false);

  if (!d.is_null() && d.contains("args")) {
    return d["args"].get<std::vector<std::string>>();
  }
  return {};
}

static const BotRequest CreatePlatformEvent(platform_message* message)
{
  return BotRequest{
    .platform = get_platform(message->platform()),
    .event    = "platform:repost",
    .username = UnescapeQuotes(message->user()),
    .data     = UnescapeQuotes(message->content()),
    .urls     = BotRequest::urls_from_string(message->urls()),
    .id       = message->id(),
    .args     = message->args(),
    .cmd      = message->cmd()
  };
}

namespace constants {
const uint8_t YOUTUBE_BOT_INDEX {0x00};
const uint8_t MASTODON_BOT_INDEX{0x01};
const uint8_t DISCORD_BOT_INDEX {0x02};
const uint8_t BLOG_BOT_INDEX    {0x03};
const uint8_t TELEGRAM_BOT_INDEX{0x04};
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
  u_bot_ptr u_bg_bot_ptr{new kbot::BlogBot{}};
  u_bot_ptr u_tg_bot_ptr{new kbot::TelegramBot{}};

  m_pool.emplace_back(std::move(u_yt_bot_ptr));
  m_pool.emplace_back(std::move(u_md_bot_ptr));
  m_pool.emplace_back(std::move(u_dc_bot_ptr));
  m_pool.emplace_back(std::move(u_bg_bot_ptr));
  m_pool.emplace_back(std::move(u_tg_bot_ptr));

  YTBot().SetCallback(&ProcessEvent);
  MDBot().SetCallback(&ProcessEvent);
  DCBot().SetCallback(&ProcessEvent);
  BLBot().SetCallback(&ProcessEvent);
  TGBot().SetCallback(&ProcessEvent);

  // YTBot().Init(); // TODO: Temporarily disabling until CPR module fixes an invalid pointer issue
  MDBot().Init();
  DCBot().Init();
  BLBot().Init();
  TGBot().Init();

  g_broker = this;
}

static const uint8_t IPC_COMMAND_INDEX{0x00};
static const uint8_t IPC_PAYLOAD_INDEX{0x01};
static const uint8_t IPC_USER_INDEX   {0x02};
static const uint8_t IPC_PARAM_NUMBER {0x03};

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
      else
      if (command == "telegram:messages")
        platform = Platform::telegram;

      SendEvent(BotRequest{platform, command, user, payload});
    }
  }
  else
  if (message->type() == ::constants::IPC_PLATFORM_TYPE)
    SendEvent(CreatePlatformEvent(static_cast<platform_message*>(message.get())));
  else
  if (message->type() == ::constants::IPC_OK_TYPE)
    kbot::log("Recv IPC OK");
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
  BLBot().Start();
  TGBot().Start();

  while (Worker::m_is_running)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
      m_condition.wait(lock,
        [this]()
        {
          return (YTBot().IsRunning() || MDBot().IsRunning() || DCBot().IsRunning() ||
                  BLBot().IsRunning() || TGBot().IsRunning());
        }
      );
    m_condition.notify_one();

    if (!m_queue.empty())
    {
      BotRequest  request  = m_queue.front();
      const auto  platform = get_platform_name(request.platform);
      if (request.event == "platform:post")
        m_outbound_queue.emplace_back(std::make_unique<platform_message>(platform,
                                                                         request.id,
                                                                         request.username,
                                                                         request.data,
                                                                         request.url_string(),
                                                                         SHOULD_REPOST));
      else
      if (request.event == "livestream inactive")
          kbot::log("YouTube bot returned no livestreams");
      else
      if (request.event == "comment")
        kbot::log(platform + " bot has new comments: " + request.data);
      else
      if (request.event == "message")
        kbot::log(platform + " bot has new messages: " + request.data);
      else
      if (request.event == "bot:success")
      {
        kbot::log(platform + " successfully handled " + request.previous_event);
        m_outbound_queue.emplace_back(std::make_unique<platform_message>(platform,
                                                                         request.id,
                                                                         request.username,
                                                                         request.data,
                                                                         request.url_string(),
                                                                         SHOULD_NOT_REPOST));
      }
      else
      if (request.event == "bot:error")
      {
        kbot::log(platform + " failed to handle " + request.previous_event);
        m_outbound_queue.emplace_back(std::make_unique<platform_error>(platform,
                                                                       request.id,
                                                                       request.username,
                                                                       request.data));
      }
      else
      if (request.event == "bot:request")
      {
        kbot::log(platform + " created a request in response to " + request.previous_event);
        m_outbound_queue.emplace_back(std::make_unique<platform_request>(platform,
                                                                         request.id,
                                                                         request.username,
                                                                         request.data,
                                                                         request.args));
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
  switch (event.platform)
  {
    case (Platform::discord):
      DCBot().HandleEvent(event);
    break;
    case (Platform::mastodon):
      MDBot().HandleEvent(event);
    break;
    case (Platform::youtube):
      YTBot().HandleEvent(event);
    break;
    case (Platform::blog):
      BLBot().HandleEvent(event);
    break;
    case (Platform::telegram):
      TGBot().HandleEvent(event);
    break;
    default:
      kbot::log("Unable to send event for unknown platform");
    break;
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
    Bot& blog_bot     = BLBot();
    Bot& tg_bot       = TGBot();

    youtube_bot .Shutdown();
    mastodon_bot.Shutdown();
    discord_bot .Shutdown();
    blog_bot    .Shutdown();
    tg_bot      .Shutdown();

    while (youtube_bot.IsRunning() || mastodon_bot.IsRunning() || discord_bot.IsRunning() ||
              blog_bot.IsRunning() ||       tg_bot.IsRunning())

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
  kbot::log("Dequeuing message: ", ::constants::IPC_MESSAGE_NAMES.at(message->type()));
  if (message->type() == ::constants::IPC_PLATFORM_TYPE)
    kbot::log("Content: ", static_cast<platform_message*>(message.get())->content().c_str());
  m_outbound_queue.pop_front();
  return std::move(message);
}

private:

Bot& YTBot()
{
  return *m_pool.at(constants::YOUTUBE_BOT_INDEX);
}

Bot& MDBot()
{
  return *m_pool.at(constants::MASTODON_BOT_INDEX);
}

Bot& DCBot()
{
  return *m_pool.at(constants::DISCORD_BOT_INDEX);
}

Bot& BLBot()
{
  return *m_pool.at(constants::BLOG_BOT_INDEX);
}

Bot& TGBot()
{
  return *m_pool.at(constants::TELEGRAM_BOT_INDEX);
}

BotPool                    m_pool;
EventQueue                 m_queue;
std::deque<u_ipc_msg_ptr>  m_outbound_queue;
bool                       m_bots_active;
std::mutex                 m_mutex;
std::condition_variable    m_condition;
};

} // namespace kbot