#pragma once

#include "interfaces/interfaces.hpp"
#include "../broker/ipc.hpp"
#include <zmq.hpp>
static const char* g_addr = "tcp://127.0.0.1:28475";
//-------------------------------------------------------------
namespace kbot
{
static platform_message BotRequestToIPC(Platform platform, const BotRequest& request)
{
  return platform_message{get_platform_name(platform), request.id,           request.username,
                          request.data,                request.url_string(), SHOULD_NOT_REPOST};
}
} // ns kbot
//-------------------------------------------------------------

class ipc_worker
{
public:
  ipc_worker()
  : ctx_(1),
    socket_(ctx_, ZMQ_DEALER)
  {
    socket_.connect(g_addr);
  }

  ~ipc_worker()
  {
    socket_.disconnect(g_addr);
  }

  void send(platform_message msg)
  {
    const auto&  payload   = msg.data();
    const size_t frame_num = payload.size();

    for (int i = 0; i < frame_num; i++)
    {
      int  flag = i == (frame_num - 1) ? 0 : ZMQ_SNDMORE;
      auto data = payload.at(i);

      zmq::message_t message{data.size()};
      std::memcpy(message.data(), data.data(), data.size());

      socket_.send(message, flag);
    }
  }

private:
  zmq::context_t ctx_;
  zmq::socket_t  socket_;
};


namespace kbot {
namespace kgram {
namespace constants {
static const uint8_t     APP_NAME_LENGTH{6};
static const std::string USER{};
} // namespace constants

static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::APP_NAME_LENGTH + 1));
}

} // ns kgram

class InstagramBot : public kbot::Worker,
                     public kbot::Bot
{
public:
InstagramBot()
: kbot::Bot{kgram::constants::USER}
{}

InstagramBot& operator=(const InstagramBot& bot)
{
  return *this;
}

virtual void Init() final
{
  return;
}

virtual void loop() final
{
  Worker::m_is_running = true;
  // TODO:
  // Get Messages
  // Send requests
}


void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}

bool HandleEvent(const BotRequest& request)
{
        bool  error = false;
        bool  reply = true;
  const auto  event = request.event;
  std::string err_msg;
  try
  {
    if (event == "livestream active" || event == "platform:repost" || event == "instagram:messages")
      m_worker.send(BotRequestToIPC(Platform::instagram, request));
  }
  catch (const std::exception& e)
  {
    err_msg += "Exception caught handling " + request.event + ": " + e.what();
    log(err_msg);
    error = true;
  }
  if (reply)
    m_send_event_fn((error) ? CreateErrorEvent(err_msg, request) : CreateSuccessEvent(request));

  return !error;
}

virtual std::unique_ptr<API> GetAPI(std::string name) final
{
  return nullptr;
}

virtual bool IsRunning() final
{
  return m_is_running;
}

virtual void Start() final
{
  if (!m_is_running)
    Worker::start();
}

virtual void Shutdown() final
{
  Worker::stop();
}

private:
  BrokerCallback m_send_event_fn;
  ipc_worker     m_worker;
};
} // namespace kgram
