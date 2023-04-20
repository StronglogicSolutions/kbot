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
#include "bot/gettr/gettr.hpp"
#include "bot/instagram/instagram.hpp"
#include "ipc.hpp"

#define OPENSSL_API_COMPAT 0x0908

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
  //------------------------------------------------------------
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

  Broker* g_broker;

  class Broker : public Worker
  {
  public:
  Broker(ipc_fail_fn _cb)
  : m_on_ipc_fail(_cb)
  {
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
      bot->Init();
    }

    g_broker = this;

    m_daemon.add_observer("botbroker", [] { kutils::log("Heartbeat timed out"); });
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
      if (!m_daemon.validate("botbroker"))
        m_on_ipc_fail();
      m_outbound_queue.emplace_back(std::make_unique<keepalive>());
    }
  }
  //------------------------------------------------------------
  static bool ProcessEvent(BotRequest event)
  {
    kutils::log("Processing event to broker's queue: ", event.to_string().c_str());
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
      return kutils::log("Failed to restart bot for platform", get_platform_name(platform).c_str());

    bot_ptr->SetCallback(&ProcessEvent);
    bot_ptr->Init();
    bot_ptr->Start();
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
          kbot::log(platform + " sending info in response to " + request.previous_event);
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
    switch (event.platform)
    {
      case (Platform::discord):   m_dc_bot.HandleEvent(event); break;
      case (Platform::mastodon):  m_md_bot.HandleEvent(event); break;
      case (Platform::youtube):   m_yt_bot.HandleEvent(event); break;
      case (Platform::blog):      m_bg_bot.HandleEvent(event); break;
      case (Platform::telegram):  m_tg_bot.HandleEvent(event); break;
      case (Platform::matrix):    m_mx_bot.HandleEvent(event); break;
      case (Platform::gettr):     m_gt_bot.HandleEvent(event); break;
      case (Platform::instagram): m_ig_bot.HandleEvent(event); break;

      default:
        kbot::log("Unable to send event for unknown platform: ", std::to_string(event.platform).c_str());
    }
  }
  //------------------------------------------------------------
  bool Shutdown()
  {
    try
    {
      for (auto&& bot : m_pool)
        bot->Shutdown();

      while (m_yt_bot.IsRunning() || m_md_bot.IsRunning() || m_dc_bot.IsRunning() ||
            m_bg_bot.IsRunning() || m_tg_bot.IsRunning() || m_mx_bot.IsRunning() ||  m_ig_bot.IsRunning())

      Worker::stop();

      return true;
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';

      return false;
    }
  }
  //------------------------------------------------------------
  const bool Poll() const
  {
    return !m_outbound_queue.empty();
  }
  //------------------------------------------------------------
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
  BotPool                   m_pool;
  EventQueue                m_queue;
  std::deque<u_ipc_msg_ptr> m_outbound_queue;
  bool                      m_bots_active;
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