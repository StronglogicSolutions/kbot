#pragma once

#include <INIReader.h>
#include "interfaces/interfaces.hpp"
#include "util/util.hpp"

namespace katrix
{
namespace constants
{
static const uint8_t     APP_NAME_LENGTH{6};
static const char*       DEFAULT_CONFIG_PATH{"config/config.ini"};
} // namespace constants

static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::APP_NAME_LENGTH  + 1));
}
//-----------------------------------------------------------------------
static std::string get_media_dir()
{
  std::string cwd = get_executable_cwd();
  return cwd.substr(0, cwd.find_last_of('/'));
}
//-----------------------------------------------------------------------
static const INIReader GetConfig()
{
  return INIReader{get_executable_cwd() + "/../" + constants::DEFAULT_CONFIG_PATH};
}
//-----------------------------------------------------------------------
static std::string GetUsername()
{
  return GetConfig().GetString("matrix_bot", "username", "");
}
//-----------------------------------------------------------------------
static std::string GetPassword()
{
  return GetConfig().GetString("matrix_bot", "password", "");
}
//-----------------------------------------------------------------------
static std::string GetRoomID()
{
  return GetConfig().GetString("matrix_bot", "room_id", "");
}
//-----------------------------------------------------------------------
static bool is_url(std::string_view s)
{
  return s.find("https://") != s.npos || s.find("http://") != s.npos;
}
} // ns katrix
namespace kbot
{
template <int64_t INTERVAL = 3000>
class timer
{
using time_point_t = std::chrono::time_point<std::chrono::system_clock>;
using ms_t         = std::chrono::milliseconds;
using duration_t   = std::chrono::duration<std::chrono::system_clock, ms_t>;

public:
  bool
  check_and_update()
  {
    if (const auto tp = now(); ready(tp))
    {
      _last = tp;
      return true;
    }
    return false;
  }

private:
  bool
  ready(const time_point_t t) const
  {
    return (std::chrono::duration_cast<ms_t>(t - _last).count() > INTERVAL);
  }
//-----------------------------------------------------------------------
  time_point_t
  now()
  {
    return std::chrono::system_clock::now();
  }
//-----------------------------------------------------------------------
  time_point_t _last{now()};
};

class MatrixBot : public kbot::Worker,
                  public kbot::Bot
{
public:
  MatrixBot()
  : kbot::Bot{"logicp"},
    m_room_id(katrix::GetRoomID()),
    m_files_to_send(0),
    m_retries(50),
    m_worker("tcp://127.0.0.1:28477", "tcp://127.0.0.1:28478",
    [this] (auto result)
    {
      klog().t("Worker returned result {}", result);
      if (!m_pending)
        klog().w("Received worker message, but nothing is pending. Last request: {}", m_last_req.id);
      else
      {
        auto   ret = BotRequest{};
        auto&& msg = m_worker.pop_last();
        if (msg->type() == ::constants::IPC_PLATFORM_TYPE)
          ret = CreatePlatformEvent(static_cast<platform_message*>(msg.get()), "bot:request");
        else
          klog().t("Received message of type {} from Katrix", ::constants::IPC_MESSAGE_NAMES.at(msg->type()));
        m_send_event_fn((result) ? CreateSuccessEvent(ret) :
                                   CreateErrorEvent("Failed to handle request", ret));
        m_pending--;
        m_posts[m_last_req.id] = true;
      }
    })
  {}
//-----------------------------------------------------------------------
MatrixBot& operator=(const MatrixBot& m)
{
  return *this;
}
//-----------------------------------------------------------------------
  virtual void Init(bool flood_protect) final
  {
    klog().d("Katrix bot init");
  }
//-----------------------------------------------------------------------
  virtual void loop() final
  {
    klog().d("Katrix bot loop");
  }
//-----------------------------------------------------------------------
  void SetCallback(BrokerCallback cb_fn) final
  {
    m_send_event_fn = cb_fn;
  }
//-----------------------------------------------------------------------
  bool HandleEvent(const BotRequest& request) final
  {
    if (!m_timer.check_and_update())
    {
      m_requests.push_back(request);
      return false;
    }

    m_last_request = request;

    try
    {
      if (m_flood_protect && post_requested(request.id))
        klogger::instance().w("{} was already requested", request.id);
      else
      {
        m_pending++;
        BotRequest outbound = request;

        if (!request.urls.empty() && katrix::is_url(request.urls.front()))
          outbound.urls = FetchFiles(request.urls, katrix::get_media_dir());

        klog().d("Sending request {} to Katrix", outbound.id);

        m_worker.send(BotRequestToIPC(Platform::instagram, outbound));
        m_last_req = outbound;
        m_posts[request.id] = false;
      }
    }
    catch(const std::exception& e)
    {
      klog().e("Exception thrown: {}", e.what());
      return false;
    }
    return true;
  }
//-----------------------------------------------------------------------
  virtual std::unique_ptr<API> GetAPI(std::string name) final
  {
    return nullptr;
  }
//-------------------------------------------------------------
  bool post_requested(const std::string& id) const
  {
    return (m_posts.find(id) != m_posts.end());
  }
//-----------------------------------------------------------------------
  virtual bool IsRunning() final
  {
    return m_is_running;
  }
//-----------------------------------------------------------------------
  virtual void Start() final
  {
    if (!m_is_running)
      Worker::start(); // maybe not necessary
  }
//-----------------------------------------------------------------------
  virtual void Shutdown() final
  {
    Worker::stop();
  }
//-----------------------------------------------------------------------
  void do_work() final
  {
    if (!m_requests.empty())
    {
      const auto request = m_requests.back();
      m_requests.pop_back();
      HandleEvent(request);
    }
  }

private:
  using files_t    = std::vector<std::string>;
  using requests_t = std::deque<BotRequest>;
  using post_map_t = std::map<std::string, bool>;

  BrokerCallback m_send_event_fn;
  BotRequest     m_last_request;
  std::string    m_room_id;
  files_t        m_files;
  uint32_t       m_files_to_send;
  uint32_t       m_retries;
  timer<3000>    m_timer;
  requests_t     m_requests;
  ipc_worker     m_worker;
  unsigned int   m_pending {0};
  BotRequest     m_last_req{};
  post_map_t     m_posts;
  bool           m_flood_protect{false};
};
} // namespace kbot
