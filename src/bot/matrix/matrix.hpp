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
      const auto&& msg = m_worker.pop_last();
      switch (msg->type())
      {
        case ::constants::IPC_PLATFORM_TYPE:                          // REQUEST
          klog().t("Sending bot:request to broker");
          m_send_event_fn(CreatePlatformEvent(static_cast<platform_message*>(msg.get()), "bot:request"));
        break;
        case ::constants::IPC_OK_TYPE:                                // SUCCESS
        {
          klog().t("Matrix bot received OK from Katrix");
          const auto id = static_cast<okay_message*>(msg.get())->id();
          const auto it = m_requests.find(id);
          if (it != m_requests.end())
            m_send_event_fn(CreateSuccessEvent(it->second));
          else
            klog().w("Received OK message, but id {} not matched", id);
        }
        break;
        case ::constants::IPC_FAIL_TYPE:                              // FAIL
        {
          klog().t("Matrix bot received FAIL from Katrix");
          const auto id = static_cast<fail_message*>(msg.get())->id();
          const auto it = m_requests.find(id);
          if (it != m_requests.end())
            m_send_event_fn(CreateErrorEvent("Katrix returned error", it->second));
          else
            klog().w("Received OK message, but id {} not matched", id);
        }
        break;
        default:                                                      // UNKNOWN
          klog().w("IPC type {} returned from worker, but not handled", ::constants::IPC_MESSAGE_NAMES.at(msg->type()));
        break;
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
    try
    {
      if (m_flood_protect && post_requested(request.id))
        klog().w("{} was already requested", request.id);
      else
      {

        BotRequest outbound = request;

        if (!request.urls.empty() && katrix::is_url(request.urls.front()))
          outbound.urls = FetchFiles(request.urls, katrix::get_media_dir());
        auto&& ipc_msg = request.event == "matrix:info" ? BotRequestToIPC<::constants::IPC_PLATFORM_INFO>(Platform::matrix, outbound) :
                                                          BotRequestToIPC                                (Platform::matrix, outbound);
        m_worker.send(std::move(ipc_msg));
        m_requests[request.id] = request;
      }
    }
    catch(const std::exception& e)
    {
      klog().e("Exception thrown: {}", e.what());
      return false;
    }
    return true;
  }
//-------------------------------------------------------------
  bool post_requested(const std::string& id) const
  {
    return (m_requests.find(id) != m_requests.end());
  }
//-----------------------------------------------------------------------
  virtual std::unique_ptr<API> GetAPI(std::string name) final
  {
    return nullptr;
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
  using requests_t = std::map<std::string, BotRequest>;
  BrokerCallback m_send_event_fn;
  std::string    m_room_id;
  files_t        m_files;
  uint32_t       m_files_to_send;
  uint32_t       m_retries;
  ipc_worker     m_worker;
  unsigned int   m_pending {0};
  requests_t     m_requests;
  bool           m_flood_protect{false};
};
} // namespace kbot
