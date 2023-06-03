#pragma once

#include "interfaces/interfaces.hpp"

//-------------------------------------------------------------
namespace kbot
{
static platform_message BotRequestToIPC(Platform platform, const BotRequest& request)
{
  return platform_message{get_platform_name(platform), request.id,           request.username,
                          request.data,                request.url_string(), SHOULD_NOT_REPOST, request.cmd, request.args, request.time};
}
} // ns kbot
//-------------------------------------------------------------
namespace kbot {
namespace kgram {
namespace constants {
static const uint8_t     APP_NAME_LENGTH{6};
static const std::string USER{};
} // namespace constants
//-------------------------------------------------------------
static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::APP_NAME_LENGTH + 1));
}
//-------------------------------------------------------------
} // ns kgram
//-------------------------------------------------------------
//-------------------------------------------------------------
//-------------------------------------------------------------
class InstagramBot : public kbot::Worker,
                     public kbot::Bot
{
public:
InstagramBot()
: kbot::Bot{kgram::constants::USER},
  m_worker("tcp://127.0.0.1:28475", "tcp://127.0.0.1:28476",
  [this] (auto result)
  {
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
//-------------------------------------------------------------
InstagramBot& operator=(const InstagramBot& bot)
{
  return *this;
}
//-------------------------------------------------------------
virtual void Init(bool flood_protect) final
{
  m_flood_protect = flood_protect;
  klogger::instance().i("Setting flood protection to {}", flood_protect);
  return;
}
//-------------------------------------------------------------
virtual void loop() final
{
  Worker::m_is_running = true;
}
//-------------------------------------------------------------
void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}
//-------------------------------------------------------------
bool post_requested(const std::string& id) const
{
  return (m_posts.find(id) != m_posts.end());
}
//-------------------------------------------------------------
bool HandleEvent(const BotRequest& request)
{
        bool  error = false;
        bool  reply = true;
  const auto  event = request.event;
  std::string err_msg;
  try
  {
    if (m_flood_protect && post_requested(request.id))
      klogger::instance().w("{} was already requested", request.id);
    else
    if (event == "livestream active" || event == "platform:repost" || event == "instagram:messages")
    {

      m_pending++;
      m_worker.send(BotRequestToIPC(Platform::instagram, request));
      m_last_req = request;
      m_posts[request.id] = false;
    }
  }
  catch (const std::exception& e)
  {
    err_msg += "Exception caught handling " + request.event + ": " + e.what();
    klogger::instance().e("{}", err_msg);
    CreateErrorEvent(err_msg, request);
    error = true;
  }
  return !error;
}
//-------------------------------------------------------------
virtual std::unique_ptr<API> GetAPI(std::string name) final
{
  return nullptr;
}
//-------------------------------------------------------------
virtual bool IsRunning() final
{
  return m_is_running;
}
//-------------------------------------------------------------
virtual void Start() final
{
  if (!m_is_running)
    Worker::start();
}
//-------------------------------------------------------------
virtual void Shutdown() final
{
  Worker::stop();
}

private:
  using post_map_t = std::map<std::string, bool>;

  BrokerCallback m_send_event_fn;
  ipc_worker     m_worker;
  BotRequest     m_last_req{};
  unsigned int   m_pending {0};
  post_map_t     m_posts;
  bool           m_flood_protect{true};
};
} // namespace kgram
