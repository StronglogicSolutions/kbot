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
    [this]
    {
      bool result = true;
      auto    ret = BotRequest{};
      auto&&  msg = m_worker.pop_last();

      switch (msg->type())
      {
        case ::constants::IPC_PLATFORM_TYPE:
          ret = CreatePlatformEvent(static_cast<platform_message*>(msg.get()), "bot:request");
        break;
        case ::constants::IPC_OK_TYPE:
        {
          const auto id = static_cast<okay_message*>(msg.get())->id();
          const auto it = m_requests.find(id);
          if (it != m_requests.end())
            ret = CreateSuccessEvent(it->second);
          else
            klog().w("Received OK message, but id {} not matched", id);
            return;
        }
        case ::constants::IPC_FAIL_TYPE:
        {
          result = false;
          const auto id = static_cast<fail_message*>(msg.get())->id();
          const auto it = m_requests.find(id);
          if (it != m_requests.end())
            ret = CreateErrorEvent("Katrix returned error", it->second);
          else
            klog().w("Received OK message, but id {} not matched", id);
            return;
        }
        break;
        default:
          klog().w("IPC type {} returned from worker, but not handled", ::constants::IPC_MESSAGE_NAMES.at(msg->type()));
          return;
      }

      m_send_event_fn(ret);
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

        klog().d("Sending request {} to Katrix with URLs", outbound.id);

        for (const auto& url : outbound.urls)
          klog().d("URL: {}", url);

        m_worker.send(BotRequestToIPC(Platform::instagram, outbound));
        m_requests[request.id] = request;
        m_posts   [request.id] = false;  // TODO: Probably don't need this anymore
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

private:
  using files_t    = std::vector<std::string>;
  using post_map_t = std::map<std::string, bool>;
  using requests_t = std::map<std::string, BotRequest>;
  BrokerCallback m_send_event_fn;
  std::string    m_room_id;
  files_t        m_files;
  uint32_t       m_files_to_send;
  uint32_t       m_retries;
  ipc_worker     m_worker;
  unsigned int   m_pending {0};
  post_map_t     m_posts;
  requests_t     m_requests;
  bool           m_flood_protect{false};
};
} // namespace kbot
