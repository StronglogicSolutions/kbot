#pragma once

#include "katrix.hpp"
#include "interfaces/interfaces.hpp"

namespace kbot {
class MatrixBot : public kbot::Worker,
                  public kbot::Bot,
                  public katrix::KatrixBot
{
public:
  MatrixBot()
  : kbot::Bot{"matrixuser"},
    katrix::KatrixBot{"matrix.org", [this](const katrix::EventID&, katrix::RequestError e)
      {
        m_send_event_fn(CreateSuccessEvent(BotRequest{}));
      }
    }
  {}

  virtual void Init() override
  {
    return;
  }

  virtual void loop() override
  {

  }

  void SetCallback(BrokerCallback cb_fn)
  {
    m_send_event_fn = cb_fn;
  }

  enum class TGCommand
  {
  message   = 0x00,
  poll      = 0x01,
  poll_stop = 0x02
  };

  bool HandleEvent(BotRequest request)
  {
    using Message = katrix::MessageType;
    static const std::string room_id = "12345";
    if (request.event == "send message")
    {
      katrix::KatrixBot::send_message(room_id, Message{request.data});
      return true;
    }
    return false;
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
    if (!m_is_running)
      Worker::start();
  }

  virtual void Shutdown() override
  {
    Worker::stop();
  }

private:
  BrokerCallback m_send_event_fn;
};
} // namespace kbot
