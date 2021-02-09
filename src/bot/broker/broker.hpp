#pragma once

#include "bot/mastodon/mastodon.hpp"
#include "bot/youtube/youtube.hpp"
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
} // namespace constants

Broker* g_broker;

class Broker : public Worker
{
public:
Broker()
{
  u_bot_ptr u_yt_bot_ptr{new kbot::YouTubeBot{}};
  u_bot_ptr u_md_bot_ptr{new kbot::MastodonBot{}};
  g_broker = this;

  m_pool.emplace_back(std::move(u_yt_bot_ptr));
  m_pool.emplace_back(std::move(u_md_bot_ptr));

  YTBot().SetCallback(&ProcessEvent);
  MDBot().SetCallback(&ProcessEvent);
  YTBot().Init();
  MDBot().Init();

}

void ProcessMessage(std::string message) {
  for (const auto& command : GetArgs(message))
  {
    bool is_equal = command == "youtube:livestream";
    if (command == "youtube:livestream")
    {
      SendEvent(Platform::youtube, command);
    }
    else
    if (command == "mastodon:comments")
    {
      SendEvent(Platform::mastodon, "comments:find");
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

  while (YTBot().IsRunning() || MDBot().IsRunning())
  {
    if (!m_queue.empty())
    {
      BotEvent event = m_queue.front();
      if (event.platform == Platform::youtube)
        if (event.name == "livestream active")
          MDBot().HandleEvent(event);
        else
        if (event.name == "livestream inactive")
          std::cout << "YouTube bot returned no livestreams" << std::endl;
      else
      if (event.platform == Platform::mastodon)
        if (event.name == "comment")
          std::cout << "Mastodon bot has new comments: " << event.data << std::endl;
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
void SendEvent(Platform platform, std::string event)
{
  if (platform == Platform::mastodon)
    MDBot().HandleEvent(BotEvent{.platform = platform, .name = event});
  else
  if (platform == Platform::youtube)
    YTBot().HandleEvent(BotEvent{.platform = platform, .name = event});
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

    youtube_bot.Shutdown();
    mastodon_bot.Shutdown();

    while (youtube_bot.IsRunning() || mastodon_bot.IsRunning())
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

BotPool    m_pool;
EventQueue m_queue;
bool       m_bots_active;
};

} // namespace kbot