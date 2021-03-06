#include "broker.hpp"
#include "channel_port.hpp"
#include <condition_variable>
#include <mutex>

const std::string OK{"OK"};

int main(int argc, char** argv)
{
  kbot::Broker      broker{};
  kbot::ChannelPort channel_port;

  broker.run();

  for (;;)
  {
    const uint8_t mask = channel_port.Poll();

    if (kbot::HasRequest(mask))
    {
      kbot::log("Incoming IPC req");
      channel_port.ReceiveIPCMessage(false);
      for (auto&& message : channel_port.GetRXMessages())
      {
        broker.ProcessMessage(std::move(message));
      }
      channel_port.SendIPCMessage(std::make_unique<okay_message>(), false);
    }

    if (kbot::HasReply(mask))
    {
      kbot::log("Incoming IPC rep");
      channel_port.ReceiveIPCMessage(true);
    }

    if (broker.Poll() && channel_port.REQReady())
    {
      kbot::log("Sending IPC req");
      channel_port.SendIPCMessage(std::move(broker.DeQueue()), true);
    }

    std::mutex                   mtx{};
    std::condition_variable      condition{};
    std::unique_lock<std::mutex> lock{mtx};
    condition.wait_for(lock, std::chrono::milliseconds(300));
  }

  broker.Shutdown();

  return 0;
}
