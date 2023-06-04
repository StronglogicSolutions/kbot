#pragma once

#include <INIReader.h>
// #include "katrix.hpp"
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

auto FetchFiles = [](const auto& urls)
{
  std::vector<std::string> fetched;
  for (const auto& url : urls) if (!url.empty()) fetched.push_back(FetchTemporaryFile(url));
  return fetched;
};

// auto error_to_string(katrix::RequestError err) -> std::string
// {
//   if (err.has_value())
//   {
//     const auto e = err.value();
//     return fmt::format("Matrix: {}\nParsed: {}\nErrcode: {}", e.matrix_error.error, e.parse_error, e.error_code);
//   }
//   return "";
// }

class MatrixBot : public kbot::Worker,
                  public kbot::Bot
                  // public katrix::KatrixBot
{
public:
  MatrixBot()
  : kbot::Bot{"logicp"},
    // katrix::KatrixBot{
    //   "matrix.org",
    //   katrix::GetUsername(),
    //   katrix::GetPassword(),
    //   [this](auto res, katrix::ResponseType type, katrix::RequestError e)
    //   {
    //     switch (type)
    //     {
    //       case (katrix::ResponseType::created):
    //         m_send_event_fn((e) ? CreateErrorEvent(error_to_string(e), m_last_request) :
    //                               CreateSuccessEvent(m_last_request));
    //       break;
    //       case (katrix::ResponseType::user_info):
    //         m_send_event_fn((e) ? CreateErrorEvent(error_to_string(e), m_last_request) :
    //                               CreateInfo(res, "presence", m_last_request));
    //       break;
    //       case (katrix::ResponseType::rooms):
    //         m_send_event_fn((e) ? CreateErrorEvent(error_to_string(e), m_last_request) :
    //                               CreateInfo(res, "rooms", m_last_request));
    //       break;
    //       case (katrix::ResponseType::file_created):
    //         katrix::klog().d("File created");
    //       break;
    //       case (katrix::ResponseType::file_uploaded):
    //         katrix::klog().d("File uploaded");
    //       break;
    //       default:
    //         katrix::klog().w("Unknown response");
    //       break;
    //     }
    //   }},
    m_room_id(katrix::GetRoomID()),
    m_files_to_send(0),
    m_retries(50),
    m_worker("tcp://127.0.0.1:28477", "tcp://127.0.0.1:28478",
    [this] (auto result) {
      if (!m_pending)
      klogger::instance().w("Received worker message, but nothing is pending. Last request: {}", m_last_req.id);
      else
      {
        m_send_event_fn((result) ? CreateSuccessEvent(m_last_req) :
                                  CreateErrorEvent("Failed to handle request", m_last_req));
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
    klog().d("Katrix logging in");
    // katrix::KatrixBot::login();
  }
//-----------------------------------------------------------------------
  virtual void loop() final
  {
    // if (katrix::KatrixBot::logged_in())
    // {
    //   katrix::klog().t("Katrix not logged in yet");
    //   while (IsRunning())
    //     katrix::KatrixBot::run();
    // }
  }
//-----------------------------------------------------------------------
  void SetCallback(BrokerCallback cb_fn) final
  {
    m_send_event_fn = cb_fn;
  }
//-----------------------------------------------------------------------
  bool HandleEvent(const BotRequest& request) final
  {
    // using Message = katrix::Msg_t;

    if (!m_timer.check_and_update())
    {
      m_requests.push_back(request);
      return false;
    }

    m_last_request = request;

    // if (!katrix::KatrixBot::logged_in())
    // {
    //   katrix::klog().t("Matrix still authenticating");
    //   std::this_thread::sleep_for(std::chrono::milliseconds(300));
    //   m_retries--;
    //   HandleEvent(request);
    // }

    try
    {
      if (m_flood_protect && post_requested(request.id))
        klogger::instance().w("{} was already requested", request.id);
      // if (request.event == "matrix:info")
      //   katrix::KatrixBot::get_user_info();
      // else
      // if (request.event == "matrix:rooms")
      //   katrix::KatrixBot::get_rooms();
      // else
      // {
      //   if (!request.urls.empty())
      //     katrix::KatrixBot::send_media_message(m_room_id, {request.data}, FetchFiles(request.urls));
      //   else
      //     katrix::KatrixBot::send_message(m_room_id, Message{request.data});
      // }
      else
      {

        m_pending++;
        m_worker.send(BotRequestToIPC(Platform::instagram, request));
        m_last_req = request;
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
    // while (!katrix::KatrixBot::logged_in());
    if (!m_is_running)
      Worker::start();
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
