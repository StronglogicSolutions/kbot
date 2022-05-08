#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

#include "bot/mastodon/mastodon.hpp"
#include "bot/youtube/youtube.hpp"
#include "bot/discord/discord.hpp"
#include "bot/telegram/telegram.hpp"
#include "bot/matrix/matrix.hpp"
#include "bot/blog/blog.hpp"
#include "ipc.hpp"

#define OPENSSL_API_COMPAT 0x0908

namespace kbot {
using bot_ptr       = Bot*;
using BotPool       = std::vector<bot_ptr>;
using EventQueue    = std::deque<BotRequest>;
using u_ipc_msg_ptr = ipc_message::u_ipc_msg_ptr;
using ipc_fail_fn   = std::function<void()>;

static const uint8_t IPC_COMMAND_INDEX{0x00};
static const uint8_t IPC_PAYLOAD_INDEX{0x01};
static const uint8_t IPC_USER_INDEX   {0x02};
static const uint8_t IPC_OPTIONS_INDEX{0x03};
static const uint8_t IPC_PARAM_NUMBER {0x03};

static std::vector<std::string> GetArgs(std::string s) {
  using json = nlohmann::json;
  json d = json::parse(s, nullptr, false);
  return (!d.is_null() && d.contains("args")) ? d["args"].get<std::vector<std::string>>() : std::vector<std::string>{};
}

static std::string CreateArgs(const std::string& s)
{
  nlohmann::json json;
  json["args"].push_back(s);
  return json.dump();
}

static std::string CreateArgs(const std::vector<std::string>& v)
{
  nlohmann::json json;
  if (v.empty()) json["args"].push_back("");
  for (const auto& s : v) json["args"].push_back(s);
  return json.dump();
}

bool ValidIPCArguments(const std::vector<std::string>& arguments)
{
  return arguments.size() >= IPC_PARAM_NUMBER;
}

bool HasOptions(const std::vector<std::string>& arguments)
{
  return arguments.size() >= IPC_PARAM_NUMBER + 1;
}

static std::string GetOptions(const std::vector<std::string>& data)
{
  std::vector<std::string> options;
  if (HasOptions(data))
    options = std::vector<std::string>{data.begin() + IPC_OPTIONS_INDEX, data.end()};
  return CreateArgs(options);
}

static const BotRequest CreatePlatformEvent(platform_message* message)
{
  return BotRequest{
    .platform = get_platform(message->platform()),
    .event    = "platform:repost",
    .username = UnescapeQuotes(message->user()),
    .data     = UnescapeQuotes(message->content()),
    .args     = message->args(),
    .urls     = BotRequest::urls_from_string(message->urls()),
    .id       = message->id(),
    .cmd      = message->cmd()
  };
}

namespace constants {
const uint8_t YOUTUBE_BOT_INDEX {0x00};
const uint8_t MASTODON_BOT_INDEX{0x01};
const uint8_t DISCORD_BOT_INDEX {0x02};
const uint8_t BLOG_BOT_INDEX    {0x03};
const uint8_t TELEGRAM_BOT_INDEX{0x04};
const uint8_t MATRIX_BOT_INDEX  {0x05};
} // namespace constants

Broker* g_broker;

class Broker : public Worker
{
public:
Broker(ipc_fail_fn _cb)
: m_on_ipc_fail(_cb)
{
  m_pool.resize(6);
  m_pool.at(constants::YOUTUBE_BOT_INDEX)  = &m_yt_bot;
  m_pool.at(constants::MASTODON_BOT_INDEX) = &m_md_bot;
  m_pool.at(constants::DISCORD_BOT_INDEX)  = &m_dc_bot;
  m_pool.at(constants::BLOG_BOT_INDEX)     = &m_bg_bot;
  m_pool.at(constants::TELEGRAM_BOT_INDEX) = &m_tg_bot;
  m_pool.at(constants::MATRIX_BOT_INDEX)   = &m_mx_bot;

  YTBot().SetCallback(&ProcessEvent);
  MDBot().SetCallback(&ProcessEvent);
  DCBot().SetCallback(&ProcessEvent);
  BLBot().SetCallback(&ProcessEvent);
  TGBot().SetCallback(&ProcessEvent);
  MXBot().SetCallback(&ProcessEvent);

  // YTBot().Init();
  MDBot().Init();
  DCBot().Init();
  BLBot().Init();
  TGBot().Init();
  MXBot().Init();

  g_broker = this;
}

void ProcessMessage(u_ipc_msg_ptr message)
{
  auto TGMessage = [](auto msg) { return msg.find("telegram") != std::string::npos; };
  auto YTMessage = [](auto msg) { return msg.find("youtube")  != std::string::npos; };
  auto MDMessage = [](auto msg) { return msg.find("mastodon") != std::string::npos; };
  auto DCMessage = [](auto msg) { return msg.find("discord")  != std::string::npos; };
  auto MXMessage = [](auto msg) { return msg.find("matrix")   != std::string::npos; };
  if (message->type() == ::constants::IPC_KIQ_MESSAGE)
  {
    kiq_message* kiq_msg = static_cast<kiq_message*>(message.get());
    auto         args    = GetArgs(kiq_msg->payload());

    if (ValidIPCArguments(args));
    {
      const auto& command = args.at(IPC_COMMAND_INDEX);
      const auto& payload = args.at(IPC_PAYLOAD_INDEX);
      const auto& user    = args.at(IPC_USER_INDEX);
      const auto& options = GetOptions(args);
      Platform    platform;

      if (YTMessage(command)) platform = Platform::youtube;
      else
      if (MDMessage(command)) platform = Platform::mastodon;
      else
      if (DCMessage(command)) platform = Platform::discord;
      else
      if (TGMessage(command)) platform = Platform::telegram;
      else
      if (MXMessage(command)) platform = Platform::matrix;

      SendEvent(BotRequest{platform, command, user, payload, options});
    }
  }
  else
  if (message->type() == ::constants::IPC_PLATFORM_TYPE)
    SendEvent(CreatePlatformEvent(static_cast<platform_message*>(message.get())));
  else
  if (message->type() == ::constants::IPC_KEEPALIVE_TYPE)
  {
    m_daemon.reset();
    if (m_daemon.validate())
      m_outbound_queue.emplace_back(std::make_unique<keepalive>());
    else
    {
      m_daemon.stop();
      m_on_ipc_fail();
    }
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
  log("Processing event to broker's queue\n", event.to_string().c_str());
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

void restart_bot(Platform platform)
{
  switch (platform)
  {
    case Platform::youtube:
      m_yt_bot = kbot::YouTubeBot{};
      m_pool.at(constants::YOUTUBE_BOT_INDEX)  = &m_yt_bot;
      YTBot().SetCallback(&ProcessEvent);
      YTBot().Init();
      YTBot().Start();
    break;
    case Platform::mastodon:
      m_md_bot = kbot::MastodonBot{};
      m_pool.at(constants::MASTODON_BOT_INDEX)  = &m_md_bot;
      MDBot().SetCallback(&ProcessEvent);
      MDBot().Init();
      MDBot().Start();
    break;
    case Platform::discord:
      m_dc_bot = kbot::DiscordBot{};
      m_pool.at(constants::DISCORD_BOT_INDEX)  = &m_dc_bot;
      DCBot().SetCallback(&ProcessEvent);
      DCBot().Init();
      DCBot().Start();
    break;
    case Platform::blog:
      m_bg_bot = kbot::BlogBot{};
      m_pool.at(constants::BLOG_BOT_INDEX)  = &m_bg_bot;
      BLBot().SetCallback(&ProcessEvent);
      BLBot().Init();
      BLBot().Start();
    break;
    case Platform::telegram:
      m_tg_bot = kbot::TelegramBot{};
      m_pool.at(constants::TELEGRAM_BOT_INDEX) = &m_tg_bot;
      MXBot().SetCallback(&ProcessEvent);
      MXBot().Init();
      MXBot().Start();
    break;
    case Platform::matrix:
      m_mx_bot = kbot::MatrixBot{};
      m_pool.at(constants::MATRIX_BOT_INDEX) = &m_mx_bot;
      MXBot().SetCallback(&ProcessEvent);
      MXBot().Init();
      MXBot().Start();
    break;
  }
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
      if (request.event == SUCCESS_EVENT)
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
      if (request.event == "bot:restart")
      {
        kbot::log(platform + " will be restarted");
        m_outbound_queue.emplace_back(std::make_unique<platform_info>(platform,
                                                                      "restart",
                                                                       "info"));
        restart_bot(request.platform);
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
      if (request.event == INFO_EVENT)
      {
        kbot::log(platform + " sending info in respones to " + request.previous_event);
        m_outbound_queue.emplace_back(std::make_unique<platform_info>(platform, request.data, request.args));
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
    case (Platform::matrix):
      MXBot().HandleEvent(event);
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
    Bot& mx_bot       = MXBot();

    youtube_bot .Shutdown();
    mastodon_bot.Shutdown();
    discord_bot .Shutdown();
    blog_bot    .Shutdown();
    tg_bot      .Shutdown();
    mx_bot      .Shutdown();

    while (youtube_bot.IsRunning() || mastodon_bot.IsRunning() || discord_bot.IsRunning() ||
              blog_bot.IsRunning() ||       tg_bot.IsRunning() || mx_bot.IsRunning())

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
  auto is_keepalive = [](auto type) { return type == ::constants::IPC_KEEPALIVE_TYPE; };

  u_ipc_msg_ptr message = std::move(m_outbound_queue.front());
  if (!is_keepalive(message->type()))
    kbot::log("Dequeuing =>", message->to_string().c_str());
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

Bot& MXBot()
{
  return *m_pool.at(constants::MATRIX_BOT_INDEX);
}

BotPool                    m_pool;
EventQueue                 m_queue;
std::deque<u_ipc_msg_ptr>  m_outbound_queue;
bool                       m_bots_active;
std::mutex                 m_mutex;
std::condition_variable    m_condition;
kbot::YouTubeBot           m_yt_bot;
kbot::MastodonBot          m_md_bot;
kbot::DiscordBot           m_dc_bot;
kbot::BlogBot              m_bg_bot;
kbot::TelegramBot          m_tg_bot;
kbot::MatrixBot            m_mx_bot;
session_daemon             m_daemon;
ipc_fail_fn                m_on_ipc_fail;
};

} // namespace kbot