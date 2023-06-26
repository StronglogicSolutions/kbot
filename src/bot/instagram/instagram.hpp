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
  [this]
  {
    const auto&& msg  = m_worker.pop_last();
          bool result = true;
          auto    ret = BotRequest{};

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
  using requests_t = std::map<std::string, BotRequest>;

  BrokerCallback m_send_event_fn;
  ipc_worker     m_worker;
  BotRequest     m_last_req{};
  unsigned int   m_pending {0};
  post_map_t     m_posts;
  requests_t     m_requests;
  bool           m_flood_protect{true};
};
} // namespace kgram
