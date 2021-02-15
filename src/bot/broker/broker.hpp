#pragma once

#include "bot/mastodon/mastodon.hpp"
#include "bot/youtube/youtube.hpp"
#include "bot/discord/discord.hpp"
#include <deque>

namespace kbot {
using u_bot_ptr  = std::unique_ptr<Bot>;
using BotPool    = std::vector<u_bot_ptr>;
using EventQueue = std::deque<BotEvent>;

inline std::vector<std::string> GetArgs(std::string s) {
  using json = nlohmann::json;
  json d = json::parse(s, nullptr, false);

  if (!d.is_null() && d.contains("args")) {
    return d["args"].get<std::vector<std::string>>();
  }
  return {};
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

void ProcessMessage(std::string message) {
  const auto frames = GetArgs(message);
  if (ValidIPCArguments(frames));
  {
    const auto command = frames.at(IPC_COMMAND_INDEX);
    const auto payload = frames.at(IPC_PAYLOAD_INDEX);

    if (command == "youtube:livestream")
    {
      SendEvent(Platform::youtube, command, payload);
    }
    else
    if (command == "mastodon:comments")
    {
      SendEvent(Platform::mastodon, "comments:find", payload);
    }
    else
    if (command == "discord:messages")
    {
      SendEvent(Platform::discord, command, payload);
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

  while (YTBot().IsRunning() || MDBot().IsRunning())
  {
    if (!m_queue.empty())
    {
      BotEvent event = m_queue.front();
      if (event.platform == Platform::youtube)
        if (event.name == "livestream active")
          {
            MDBot().HandleEvent(event);
            DCBot().HandleEvent(event);
          }
        else
        if (event.name == "livestream inactive")
          std::cout << "YouTube bot returned no livestreams" << std::endl;
      else
      if (event.platform == Platform::mastodon)
        if (event.name == "comment")
          std::cout << "Mastodon bot has new comments: " << event.data << std::endl;
      else
      if (event.platform == Platform::discord)
        if (event.name == "message")
          std::cout << "Discord bot has new messages: " << event.data << std::endl;
      else
        throw std::runtime_error{"No bot to handle this type of event"};

      m_queue.pop_front();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
}

/**
 * @brief
 *
 * @param platform
 * @param event
 */
void SendEvent(Platform platform, std::string event, std::string payload = "")
{
  if (platform == Platform::mastodon)
    MDBot().HandleEvent(BotEvent{.platform = platform, .name = event, .data = payload});
  else
  if (platform == Platform::youtube)
    YTBot().HandleEvent(BotEvent{.platform = platform, .name = event, .data = payload});
  else
  if (platform == Platform::discord)
    DCBot().HandleEvent(BotEvent{.platform = platform, .name = event, .data = payload});

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

BotPool    m_pool;
EventQueue m_queue;
bool       m_bots_active;
};

} // namespace kbot