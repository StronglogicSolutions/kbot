#pragma once

#include <INIReader.h>
#include "katrix.hpp"
#include "interfaces/interfaces.hpp"
#include "util/util.hpp"

namespace katrix
{
namespace constants {
static const uint8_t     APP_NAME_LENGTH{6};
static const char*       DEFAULT_CONFIG_PATH{"config/config.ini"};
} // namespace constants

static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::APP_NAME_LENGTH  + 1));
}

static const INIReader GetConfig()
{
  return INIReader{get_executable_cwd() + "/../" + constants::DEFAULT_CONFIG_PATH};
}

static std::string GetUsername()
{
  return GetConfig().GetString("matrix_bot", "username", "");
}

static std::string GetPassword()
{
  return GetConfig().GetString("matrix_bot", "password", "");
}

static std::string GetRoomID()
{
  return GetConfig().GetString("matrix_bot", "room_id", "");
}

} // ns katrix
namespace kbot {
class MatrixBot : public kbot::Worker,
                  public kbot::Bot,
                  public katrix::KatrixBot
{
public:
  MatrixBot()
  : kbot::Bot{"logicp"},
    katrix::KatrixBot{
      "matrix.org",
      katrix::GetUsername(),
      katrix::GetPassword(),
      [this](auto res, katrix::ResponseType type, katrix::RequestError e)
      {
        switch (type)
        {
          case (katrix::ResponseType::created):
            m_send_event_fn((e) ? CreateErrorEvent(katrix::error_to_string(e), m_last_request) :
                                  CreateSuccessEvent(m_last_request));
          break;
          case (katrix::ResponseType::user_info):
            m_send_event_fn((e) ? CreateErrorEvent(katrix::error_to_string(e), m_last_request) :
                                  CreateInfo(res, "presence", m_last_request));
          break;
          case (katrix::ResponseType::rooms):
            m_send_event_fn((e) ? CreateErrorEvent(katrix::error_to_string(e), m_last_request) :
                                  CreateInfo(res, "rooms", m_last_request));
          break;
          // case (katrix::ResponseType::file_created):
          //   m_files.push_back(res);
          //   if (m_files.size() == m_files_to_send)
          //   {
          //     m_files_to_send = 0;
          //     HandleEvent(m_last_request);
          //   }
          // break;
          default:
            katrix::log("Unknown response");
          break;
        }
      }},
    m_room_id(katrix::GetRoomID()),
    m_files_to_send(0)
  {}

  virtual void Init() override
  {
    katrix::KatrixBot::login();
  }

  virtual void loop() override
  {
    if (katrix::KatrixBot::logged_in())
    {
      while (IsRunning())
        katrix::KatrixBot::run();
    }
  }

  void SetCallback(BrokerCallback cb_fn)
  {
    m_send_event_fn = cb_fn;
  }

  bool HandleEvent(BotRequest request)
  {
    using Message = katrix::MessageType;

    auto FetchFiles = [](auto urls)
    {
      std::vector<std::string> fetched_uris;
      for (const auto& url : urls) fetched_uris.push_back(FetchTemporaryFile(url));
      return fetched_uris;
    };

    m_last_request = request;
    try
    {
      if (request.event == "matrix:info")
        katrix::KatrixBot::get_user_info();
      else
      if (request.event == "matrix:rooms")
        katrix::KatrixBot::get_rooms();
      else
      {
        if (!request.urls.empty())
          katrix::KatrixBot::send_media_message(m_room_id, {request.data}, FetchFiles(request.urls));
        else
          katrix::KatrixBot::send_message(m_room_id, Message{request.data});
      }
    }
    catch(const std::exception& e)
    {
      log("Exception thrown: ", e.what());
      return false;
    }
    return true;
  }

  virtual std::unique_ptr<API> GetAPI(std::string name) override
  {
    return nullptr;
  }

  virtual bool IsRunning() override
  {
    return m_is_running;
  }

  virtual void Start() override
  {
    while (!katrix::KatrixBot::logged_in());
    if (!m_is_running)
      Worker::start();
  }

  virtual void Shutdown() override
  {
    Worker::stop();
  }

private:
  using files_t = std::vector<std::string>;

  BrokerCallback m_send_event_fn;
  BotRequest     m_last_request;
  std::string    m_room_id;
  files_t        m_files;
  uint32_t       m_files_to_send;
};
} // namespace kbot
