#pragma once

#include "interfaces/interfaces.hpp"
namespace kiq
{
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
static constexpr const char* const s_name = "instagram_bot";

public:
InstagramBot()
: kbot::Bot{kgram::constants::USER},
  m_worker("tcp://127.0.0.1:28475", "tcp://127.0.0.1:28476",
  [this]
  {
    const auto&& msg = m_worker.pop_last();
    switch (msg->type())
    {
      case kiq::constants::IPC_PLATFORM_TYPE:                          // REQUEST
        klog().t("Sending bot:request to broker");
        m_send_event_fn(CreatePlatformEvent(static_cast<platform_message*>(msg.get()), "platform:post"));
      break;
      case kiq::constants::IPC_OK_TYPE:                                // SUCCESS
      {
        klog().t("Matrix bot received OK from KGram");
        const auto id = static_cast<okay_message*>(msg.get())->id();
        const auto it = m_requests.find(id);
        if (it != m_requests.end())
          m_send_event_fn(CreateSuccessEvent(it->second));
        else
          klog().w("Received OK message, but id {} not matched", id);
      }
      break;
      case kiq::constants::IPC_FAIL_TYPE:                              // FAIL
      {
        klog().t("Matrix bot received FAIL from KGram");
        const auto id = static_cast<fail_message*>(msg.get())->id();
        const auto it = m_requests.find(id);
        if (it != m_requests.end())
          m_send_event_fn(CreateErrorEvent("Katrix returned error", it->second));
        else
          klog().w("Received OK message, but id {} not matched", id);
      }
      break;
      case kiq::constants::IPC_KEEPALIVE_TYPE:
        if (!m_daemon.validate(s_name))
          klog().e("KGram timed out");
        m_daemon.reset();
      default:                                                      // UNKNOWN
        klog().w("IPC type {} returned from worker, but not handled",
          kiq::constants::IPC_MESSAGE_NAMES.at(msg->type()));
      break;
    }
  })
{
  m_daemon.add_observer(s_name, [this]
  {
    klog().d("Keep alive was not renewed. Resetting {} worker", s_name);
    m_worker.reset();
  });
}
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
  static timer<86400000> timer;
  if (timer.check_and_update())
    m_worker.reset();
}
//-------------------------------------------------------------
void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}
//-------------------------------------------------------------
bool post_requested(const std::string& id) const
{
  return (m_requests.find(id) != m_requests.end());
}
//-------------------------------------------------------------
bool HandleEvent(const BotRequest& request)
{
  m_requests[request.id] = request;
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

      m_worker.send(BotRequestToIPC(Platform::instagram, request));
      m_requests[request.id] = request;
    }
    else
    if (event == "instagram:query")
    {
      m_worker.send(BotRequestToIPC<kiq::constants::IPC_KIQ_MESSAGE>(Platform::instagram, request));
      m_requests[request.id] = request;
    }
    else
    if (event == "instagram:status")
    {
      m_worker.send(BotRequestToIPC<kiq::constants::IPC_STATUS>(Platform::instagram, request));
      m_requests[request.id] = request;
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
  using requests_t = std::map<std::string, BotRequest>;

  BrokerCallback m_send_event_fn;
  ipc_worker     m_worker;
  requests_t     m_requests;
  bool           m_flood_protect{true};
  session_daemon m_daemon;
};
} // namespace kgram
} // ns kiq
