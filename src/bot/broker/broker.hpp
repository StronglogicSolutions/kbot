#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

#include <INIReader.h>

#include "bot/mastodon/mastodon.hpp"
#include "bot/youtube/youtube.hpp"
#include "bot/discord/discord.hpp"
#include "bot/telegram/telegram.hpp"
#include "bot/matrix/matrix.hpp"
#include "bot/blog/blog.hpp"
#include "bot/gettr/gettr.hpp"
#include "bot/instagram/instagram.hpp"
#include "ipc.hpp"

#include <logger.hpp>

#define OPENSSL_API_COMPAT 0x0908

using namespace kiq::log;

namespace kbot
{
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

  static std::vector<std::string> GetArgs(std::string s)
  {
    using json = nlohmann::json;
    json d = json::parse(s, nullptr, false);
    return (!d.is_null() && d.contains("args")) ? d["args"].get<std::vector<std::string>>() : std::vector<std::string>{};
  }
  //------------------------------------------------------------
  static std::string CreateArgs(const std::string& s)
  {
    nlohmann::json json;
    json["args"].push_back(s);
    return json.dump();
  }
  //------------------------------------------------------------
  static std::string CreateArgs(const std::vector<std::string>& v)
  {
    nlohmann::json json;
    if (v.empty()) json["args"].push_back("");
    for (const auto& s : v) json["args"].push_back(s);
    return json.dump();
  }
  //------------------------------------------------------------
  bool ValidIPCArguments(const std::vector<std::string>& arguments)
  {
    return arguments.size() >= IPC_PARAM_NUMBER;
  }
  //------------------------------------------------------------
  bool HasOptions(const std::vector<std::string>& arguments)
  {
    return arguments.size() >= IPC_PARAM_NUMBER + 1;
  }
  //------------------------------------------------------------
  static std::string GetOptions(const std::vector<std::string>& data)
  {
    std::vector<std::string> options;
    if (HasOptions(data))
      options = std::vector<std::string>{data.begin() + IPC_OPTIONS_INDEX, data.end()};
    return CreateArgs(options);
  }

  Broker* g_broker;

  class Broker : public Worker
  {
  public:
  Broker(ipc_fail_fn _cb)
  : m_on_ipc_fail(_cb)
  {
    const auto execpath = get_executable_cwd();
    const auto config   = INIReader{execpath + "/../config/config.ini"};

    klog().d("Exec path is {}", execpath);

    if (config.ParseError() < 0)
      klog().i("Failed to load config");
    else
      m_flood_protect = config.GetBoolean("broker", "flood_protect", false);

    m_pool.push_back(&m_yt_bot);
    m_pool.push_back(&m_md_bot);
    m_pool.push_back(&m_dc_bot);
    m_pool.push_back(&m_bg_bot);
    m_pool.push_back(&m_tg_bot);
    m_pool.push_back(&m_mx_bot);
    m_pool.push_back(&m_gt_bot);
    m_pool.push_back(&m_ig_bot);

    for (auto&& bot : m_pool)
    {
      bot->SetCallback(&ProcessEvent);
      bot->Init(m_flood_protect);
    }

    g_broker = this;

    m_daemon.add_observer("botbroker", [] { klogger::instance().w("Heartbeat timed out"); });
  }
  //------------------------------------------------------------
  void ProcessMessage(u_ipc_msg_ptr message)
  {
    auto TGMessage = [](auto msg) { return msg.find("telegram") != std::string::npos; };
    auto YTMessage = [](auto msg) { return msg.find("youtube")  != std::string::npos; };
    auto MDMessage = [](auto msg) { return msg.find("mastodon") != std::string::npos; };
    auto DCMessage = [](auto msg) { return msg.find("discord")  != std::string::npos; };
    auto MXMessage = [](auto msg) { return msg.find("matrix")   != std::string::npos; };
    auto GTMessage = [](auto msg) { return msg.find("gettr")    != std::string::npos; };
    auto IGMessage = [](auto msg) { return msg.find("instagram")!= std::string::npos; };

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

        klog().d("Received KIQ message with command: {}, payload: {}, user: {}, options: {}",
          command, payload, user, options);
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
        else
        if (GTMessage(command)) platform = Platform::gettr;
        else
        if (IGMessage(command)) platform = Platform::instagram;

        klog().d("Sending bot request to {}", get_platform_name(platform));
        SendEvent(BotRequest{platform, command, user, payload, options});
      }
    }
    else
    if (message->type() == ::constants::IPC_PLATFORM_TYPE)
      SendEvent(CreatePlatformEvent(static_cast<platform_message*>(message.get())));
    else
    if (message->type() == ::constants::IPC_KEEPALIVE_TYPE)
    {
      if (!m_daemon.validate("botbroker"))
        m_on_ipc_fail();
      m_daemon.reset();
      m_outbound_queue.emplace_back(std::make_unique<keepalive>());
    }
  }
  //------------------------------------------------------------
  static bool ProcessEvent(BotRequest event)
  {
    klogger::instance().d("Processing event to broker's queue: {}", event.to_string());
    if (g_broker != nullptr)
    {
      g_broker->enqueue(event);
      return true;
    }
    return false;
  }
  //------------------------------------------------------------
  void enqueue(BotRequest event)
  {
    m_queue.emplace_back(event);
  }
  //------------------------------------------------------------
  void run()
  {
    Worker::start();
  }
  //------------------------------------------------------------
  void restart_bot(Platform platform)
  {
    Bot* bot_ptr = nullptr;
    switch (platform)
    {
      case Platform::youtube:
        m_yt_bot = kbot::YouTubeBot{};
        bot_ptr  = &m_yt_bot;
      break;
      case Platform::mastodon:
        m_md_bot = kbot::MastodonBot{};
        bot_ptr  = &m_md_bot;
      break;
      case Platform::discord:
        m_dc_bot = kbot::DiscordBot{};
        bot_ptr  = &m_dc_bot;
      break;
      case Platform::blog:
        m_bg_bot = kbot::BlogBot{};
        bot_ptr  = &m_bg_bot;
      break;
      case Platform::telegram:
        m_tg_bot = kbot::TelegramBot{};
        bot_ptr  = &m_tg_bot;
      break;
      case Platform::matrix:
        m_mx_bot = kbot::MatrixBot{};
        bot_ptr  = & m_mx_bot;
      break;
      case Platform::gettr:
        m_gt_bot = kbot::GettrBot{};
        bot_ptr  = &m_gt_bot;
      break;
      case Platform::instagram:
        m_ig_bot = kbot::InstagramBot{};
        bot_ptr  = &m_ig_bot;
    }
    if (!bot_ptr)
      klogger::instance().i("Failed to restart bot for platform {}", get_platform_name(platform));
    else
    {
      bot_ptr->SetCallback(&ProcessEvent);
      bot_ptr->Init(m_flood_protect);
      bot_ptr->Start();
    }
  }
  //------------------------------------------------------------
  virtual void loop() override
  {
    for (auto&& bot : m_pool)
      bot->Start();

    while (Worker::m_is_running)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock,
          [this]()
          {
            return (m_yt_bot.IsRunning() || m_md_bot.IsRunning() || m_dc_bot.IsRunning() ||
                    m_bg_bot.IsRunning() || m_tg_bot.IsRunning() || m_mx_bot.IsRunning()); // GettrBot not included
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
                                                                          SHOULD_REPOST,
                                                                          request.cmd,
                                                                          request.args,
                                                                          request.time));
        else
        if (request.event == "livestream inactive")
            klogger::instance().w("{} bot returned no livestreams", platform);
        else
        if (request.event == "comment")
          klogger::instance().i("{} bot has new comments: {}", platform.c_str(), request.data);
        else
        if (request.event == "message")
          klogger::instance().i("{} bot has new messages: {}", platform.c_str(), request.data);
        else
        if (request.event == SUCCESS_EVENT)
        {
          klogger::instance().i("{} successfully handled {}", platform.c_str(), request.previous_event);
          m_outbound_queue.emplace_back(std::make_unique<platform_message>(platform,
                                                                          request.id,
                                                                          request.username,
                                                                          request.data,
                                                                          request.url_string(),
                                                                          SHOULD_NOT_REPOST,
                                                                          request.cmd,
                                                                          request.args,
                                                                          request.time));
        }
        else
        if (request.event == "bot:error")
        {
          klogger::instance().i("{} failed to handle {}", platform.c_str(), request.previous_event);
          m_outbound_queue.emplace_back(std::make_unique<platform_error>(platform,
                                                                        request.id,
                                                                        request.username,
                                                                        request.data));
        }
        else
        if (request.event == "bot:restart")
        {
          klogger::instance().i("{} will be restarted", platform);
          m_outbound_queue.emplace_back(std::make_unique<platform_info>(platform,
                                                                        "restart",
                                                                        "info"));
          restart_bot(request.platform);
        }
        else
        if (request.event == "bot:request")
        {
          klogger::instance().i("{} created a request in response to {}", platform, request.previous_event);
          m_outbound_queue.emplace_back(std::make_unique<platform_request>(platform,
                                                                          request.id,
                                                                          request.username,
                                                                          request.data,
                                                                          request.args));
        }
        else
        if (request.event == INFO_EVENT)
        {
          klogger::instance().i("{} sending info in response to {}", platform, request.previous_event);
          m_outbound_queue.emplace_back(std::make_unique<platform_info>(platform, request.data, request.args));
        }
        else
          SendEvent(request);

        m_queue.pop_front();
      }

      for (const auto& bot : m_pool)
        bot->do_work();

      m_condition.wait_for(lock, std::chrono::milliseconds(300));
    }
  }
  //------------------------------------------------------------
  void SendEvent(const BotRequest& event)
  {
    static const std::map<Platform, std::function<bool(())>> methods{
      {(Platform::discord),   [this, &event] { return m_dc_bot.HandleEvent(event);}},
      {(Platform::mastodon),  [this, &event] { return m_md_bot.HandleEvent(event);}},
      {(Platform::youtube),   [this, &event] { return m_yt_bot.HandleEvent(event);}},
      {(Platform::blog),      [this, &event] { return m_bg_bot.HandleEvent(event);}},
      {(Platform::telegram),  [this, &event] { return m_tg_bot.HandleEvent(event);}},
      {(Platform::matrix),    [this, &event] { return m_mx_bot.HandleEvent(event);}},
      {(Platform::gettr),     [this, &event] { return m_gt_bot.HandleEvent(event);}},
      {(Platform::instagram), [this, &event] { return m_ig_bot.HandleEvent(event);}}};

    if (methods.at(event.platform)())
      klog().i("Request handled by {}", get_platform_name(event.platform));
    else
      klog().e("Request failed by {}",  get_platform_name(event.platform));
  }
  //------------------------------------------------------------
  bool Shutdown()
  {
    try
    {
      for (auto&& bot : m_pool)
        bot->Shutdown();
      Worker::stop();

      if (m_yt_bot.IsRunning() || m_md_bot.IsRunning() || m_dc_bot.IsRunning() ||
          m_bg_bot.IsRunning() || m_tg_bot.IsRunning() || m_mx_bot.IsRunning() ||  m_ig_bot.IsRunning())
        klog().w("Bot(s) still running");

      return true;
    }
    catch(const std::exception& e)
    {
      klogger::instance().e("Exception caught during shutdown: {}", e.what());
    }

    return false;
  }
  //------------------------------------------------------------
  const bool Poll() const
  {
    return !m_outbound_queue.empty();
  }
  //------------------------------------------------------------
  u_ipc_msg_ptr DeQueue()
  {
    u_ipc_msg_ptr message = std::move(m_outbound_queue.front());
    m_outbound_queue.pop_front();

    if (message->type() != ::constants::IPC_KEEPALIVE_TYPE)
      klogger::instance().d("Dequeued: {}", message->to_string());

    return std::move(message);
  }

private:
  BotPool                   m_pool;
  EventQueue                m_queue;
  std::deque<u_ipc_msg_ptr> m_outbound_queue;
  bool                      m_bots_active;
  bool                      m_flood_protect{true};
  std::mutex                m_mutex;
  std::condition_variable   m_condition;
  kbot::YouTubeBot          m_yt_bot;
  kbot::MastodonBot         m_md_bot;
  kbot::DiscordBot          m_dc_bot;
  kbot::BlogBot             m_bg_bot;
  kbot::TelegramBot         m_tg_bot;
  kbot::MatrixBot           m_mx_bot;
  kbot::GettrBot            m_gt_bot;
  kbot::InstagramBot        m_ig_bot;
  session_daemon            m_daemon;
  ipc_fail_fn               m_on_ipc_fail;
};

} // namespace kbot