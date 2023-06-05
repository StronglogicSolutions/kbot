#pragma once

#include <string>
#include <thread>
#include <memory>
#include <vector>

#include "util/util.hpp"
#include <bot/broker/ipc.hpp>
#include <zmq.hpp>
#include <logger.hpp>

class API {
 public:
  virtual std::string GetType() = 0;
  virtual bool        TestMode() {
    return true;
  }
};

namespace kbot {

using namespace kiq::log;

class  Broker;

enum Platform
{
  youtube   = 0x00,
  mastodon  = 0x01,
  discord   = 0x02,
  blog      = 0x03,
  telegram  = 0x04,
  matrix    = 0x05,
  gettr     = 0x06,
  instagram = 0x07,
  unknown   = 0x08
};


static const char* g_YouTube_str   = "YouTube";
static const char* g_Mastodon_str  = "Mastodon";
static const char* g_Discord_str   = "Discord";
static const char* g_Blog_str      = "Blog";
static const char* g_Telegram_str  = "Telegram";
static const char* g_Matrix_str    = "Matrix";
static const char* g_GETTR_str     = "GETTR";
static const char* g_Instagram_str = "Instagram";
static const char* g_Unknown_str   = "Unknown";

static enum Platform get_platform(const std::string& name)
{
  if (name == g_Discord_str)   return Platform::discord;
  if (name == g_Mastodon_str)  return Platform::mastodon;
  if (name == g_YouTube_str)   return Platform::youtube;
  if (name == g_Blog_str)      return Platform::blog;
  if (name == g_Telegram_str)  return Platform::telegram;
  if (name == g_Matrix_str)    return Platform::matrix;
  if (name == g_GETTR_str)     return Platform::gettr;
  if (name == g_Instagram_str) return Platform::instagram;
  return Platform::unknown;
}

static const std::string get_platform_name(Platform platform)
{
  switch (platform)
  {
    case Platform::youtube:   return g_YouTube_str;
    case Platform::mastodon:  return g_Mastodon_str;
    case Platform::discord:   return g_Discord_str;
    case Platform::blog:      return g_Blog_str;
    case Platform::telegram:  return g_Telegram_str;
    case Platform::matrix:    return g_Matrix_str;
    case Platform::gettr:     return g_GETTR_str;
    case Platform::instagram: return g_Instagram_str;
  }

  return g_Unknown_str;
};

struct BotRequest
{
Platform                 platform;
std::string              event;
std::string              username;
std::string              data;
std::string              args;
std::vector<std::string> urls;
std::string              id;
std::string              previous_event;
uint32_t                 cmd{0x00};
std::string              time;

const std::string to_string() const
{
  auto text = (data.size() > 30) ? data.substr(0, 30) : data;
  return "Platform:       " + get_platform_name(platform) + '\n' +
         "Event:          " + event + '\n' +
         "Username:       " + username + '\n' +
         "Data:           " + text + '\n' +
         "Urls:           " + url_string() + '\n' +
         "Id:             " + id + '\n' +
         "Previous_event: " + previous_event + '\n' +
         "Args:           " + args + '\n' +
         "Cmd:            " + std::to_string(cmd) + '\n';
}

const std::string url_string() const
{
  std::string output;
  std::string delim{};

  for (const auto& url : urls)
  {
    output += delim + url;
    delim = ">";
  }

  return output;
}

static const std::vector<std::string> urls_from_string(std::string input_string)
{
  std::string s = kbot::UnescapeQuotes(input_string);
  static const std::string delim{'>'};
  std::vector<std::string> urls{};

  if (!s.empty())
  {
    auto pos = s.find_first_of(delim);

    while (pos != s.npos)
    {
      urls.emplace_back(s.substr(0, pos));
      s = s.substr(pos + 1);
      pos = s.find_first_of(delim);
    }

    urls.emplace_back(s);
  }
  return urls;
}
};

static const bool SHOULD_REPOST{true};
static const bool SHOULD_NOT_REPOST{false};
static const std::string SUCCESS_EVENT{"bot:success"};
static const std::string INFO_EVENT   {"bot:info"};
static const std::string RESTART_EVENT{"bot:restart"};

static const BotRequest CreateSuccessEvent(const BotRequest& previous_event)
{
  kbot::log("Creating success event");
  return BotRequest{
    .platform       = previous_event.platform,
    .event          = SUCCESS_EVENT,
    .username       = previous_event.username,
    .data           = previous_event.data,
    .args           = previous_event.args,
    .urls           = previous_event.urls,
    .id             = previous_event.id,
    .previous_event = previous_event.event
  };
}

static const BotRequest CreateInfo(const std::string& info, const std::string& type, const BotRequest& previous_event)
{
  kbot::log("Creating info event");
  return BotRequest{
    .platform       = previous_event.platform,
    .event          = INFO_EVENT,
    .username       = previous_event.username,
    .data           = info,
    .args           = type,
    .urls           = previous_event.urls,
    .id             = previous_event.id,
    .previous_event = previous_event.event
  };
}

static const BotRequest CreateRequest(const std::string& message, const std::string& args, const BotRequest& previous_event)
{
  kbot::log("Creating request with message ", message.c_str(), " and args ", args.c_str());
  return BotRequest{
    .platform       = previous_event.platform,
    .event          = "bot:request",
    .username       = previous_event.username,
    .data           = message,
    .args           = args,
    .id             = previous_event.id,
    .previous_event = previous_event.event
  };
}

static const BotRequest CreateErrorEvent(const std::string& error_message, const BotRequest& previous_event)
{
  kbot::log("Creating error event");
  return BotRequest{
    .platform = previous_event.platform,
    .event    = "bot:error",
    .username = previous_event.username,
    .data     = error_message,
    .urls     = previous_event.urls,
    .id       = previous_event.id
  };
}

using BrokerCallback = bool(*)(BotRequest event);

class Bot
{
public:
  Bot(std::string name)
  : m_name(name) {}

  virtual ~Bot() {}
  std::string GetName() { return m_name; }

  virtual std::unique_ptr<API> GetAPI(std::string name)              = 0;
  virtual void                 SetCallback(BrokerCallback cb_fn_ptr) = 0;
  virtual bool                 HandleEvent(const BotRequest& event)  = 0;
  virtual bool                 IsRunning()                           = 0;
  virtual void                 Start()                               = 0;
  virtual void                 Shutdown()                            = 0;
  virtual void                 Init(bool flood_protect)              = 0;
  virtual void                 do_work() { /* no-op */ };

private:
  std::string m_name;
};

class Worker
{
public:
  Worker()
  : m_is_running(false) {}

  virtual void start()
  {
    m_is_running = true;
    m_thread = std::thread(Worker::run, this);
  }

  static void run(void* worker)
  {
    static_cast<Worker*>(worker)->loop();
  }

  void stop()
  {
    m_is_running = false;
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  bool        m_is_running;

protected:
  virtual void loop() = 0;

private:
  std::thread m_thread;
};

struct PlatformQuery
{
  Platform                 platform;
  std::string              subject;
  std::string              text;
  std::vector<std::string> urls;
};

using observer_t = std::function<void(bool)>;
class ipc_worker
{
public:
  explicit ipc_worker(const char* addr, const char* recv_addr, const observer_t& cb)
  : ctx_(1),
    tx_(ctx_, ZMQ_DEALER),
    rx_(ctx_, ZMQ_ROUTER),
    cb_(cb),
    addr_(addr),
    recv_addr_(recv_addr)
  {
    tx_.set(zmq::sockopt::linger, 0);
    rx_.set(zmq::sockopt::linger, 0);
    tx_.set(zmq::sockopt::tcp_keepalive, 1);
    rx_.set(zmq::sockopt::tcp_keepalive, 1);
    tx_.set(zmq::sockopt::tcp_keepalive_idle,  300);
    tx_.set(zmq::sockopt::tcp_keepalive_intvl, 300);
    rx_.set(zmq::sockopt::tcp_keepalive_idle,  300);
    rx_.set(zmq::sockopt::tcp_keepalive_intvl, 300);
    tx_.connect(addr_);
    rx_.bind   (recv_addr_);

    fut_ = std::async(std::launch::async, [this] {
      while (active_)
        recv(); });

  }
  //-------------------------------------------------------------
  ~ipc_worker()
  {
    tx_.disconnect(addr_);
    rx_.disconnect(recv_addr_);
  }
  //-------------------------------------------------------------
  void send(platform_message msg)
  {
    const auto&  payload   = msg.data();
    const size_t frame_num = payload.size();

    for (int i = 0; i < frame_num; i++)
    {
      auto flag = i == (frame_num - 1) ? zmq::send_flags::none : zmq::send_flags::sndmore;
      auto data = payload.at(i);

      zmq::message_t message{data.size()};
      std::memcpy(message.data(), data.data(), data.size());

      tx_.send(message, flag);
    }
  }
  //-------------------------------------------------------------
  void recv()
  {
    using buffers_t = std::vector<ipc_message::byte_buffer>;

    zmq::message_t identity;
    if (!rx_.recv(identity) || identity.empty())
    {
      klogger::instance().e("Socket failed to receive identity");
      return;
    }

    buffers_t      buffer;
    zmq::message_t msg;
    int            more_flag{1};

    while (more_flag && rx_.recv(msg))
    {
      more_flag = rx_.get(zmq::sockopt::rcvmore);
      buffer.push_back({static_cast<char*>(msg.data()), static_cast<char*>(msg.data()) + msg.size()});
    }
    msgs_.push_back(DeserializeIPCMessage(std::move(buffer)));
    klogger::instance().d("IPC message received");
    cb_(true);
  }

private:
  using ipc_msgs_t = std::vector<ipc_message::u_ipc_msg_ptr>;

  zmq::context_t    ctx_;
  zmq::socket_t     tx_;
  zmq::socket_t     rx_;
  std::future<void> fut_;
  bool              active_{true};
  ipc_msgs_t        msgs_;
  observer_t        cb_;
  std::string       addr_;
  std::string       recv_addr_;
};

//-------------------------------------------------------------
static platform_message BotRequestToIPC(Platform platform, const BotRequest& request)
{
  return platform_message{get_platform_name(platform), request.id,           request.username,
                          request.data,                request.url_string(), SHOULD_NOT_REPOST, request.cmd, request.args, request.time};
}

} // namespace kbot
