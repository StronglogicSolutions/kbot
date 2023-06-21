#pragma once

#include "interfaces/interfaces.hpp"
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
    const auto&& msg = m_worker.pop_last();
    if (!msg)
    {
      klog().w("Worker provided result {} but no message found", result);
      return;
    }

    BotRequest ret{kbot::Platform::instagram, (result) ? SUCCESS_EVENT : ERROR_EVENT};
    const auto type = msg->type();

    klog().t("Worker returned result {}", result);

    if (!m_pending)
      klog().w("Received worker message, but nothing is pending. Last request: {}", m_last_req.id);
    else
    if (type == ::constants::IPC_PLATFORM_TYPE)
    {
      const auto* msg_ptr = static_cast<platform_message*>(msg.get());
      if (!m_pending && !msg_ptr->repost())
        klog().w("No replies pending. Ignoring this message from worker: {}", msg_ptr->to_string());
      else
        m_send_event_fn(CreatePlatformEvent(msg_ptr, "platform:post"));
      return;
    }

    m_send_event_fn((result) ? CreateSuccessEvent(ret) :
                               CreateErrorEvent("Failed to handle request", ret));
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
  klog().i("Setting flood protection to {}", flood_protect);
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
    if (event == "livestream active" || event == "platform:repost" || event == "instagram:messages")
    {
      if (m_flood_protect && post_requested(request.id))
      {
        klogger::instance().w("{} was already requested", request.id);
        return false;
      }

      m_pending++;
      m_worker.send(BotRequestToIPC(Platform::instagram, request));
      m_last_req = request;
      m_posts[request.id] = false;
    }
    else
    if (event == "instagram:query")
    {
      m_pending++;
      m_worker.send(BotRequestToIPC<::constants::IPC_KIQ_MESSAGE>(Platform::instagram, request));
      m_last_req = request;
      m_posts[request.id] = false;
    }
  }
  catch (const std::exception& e)
  {
    err_msg += "Exception caught handling " + request.event + ": " + e.what();
    klogger::instance().e(err_msg);
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
