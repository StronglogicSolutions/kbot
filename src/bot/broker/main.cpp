#include "broker.hpp"
#include "channel_port.hpp"
#include <condition_variable>
#include <mutex>

static void sig_pipe_handler(int signal)
{
  static const auto path = kutils::GetCWD(6) + "/sig_caught.log";
  const std::string message{"Latest signal " + std::to_string(signal) + " at " + kutils::get_simple_datetime()};
  kutils::SaveToFile(message, path);
}

int main(int argc, char** argv)
{

  signal(SIGPIPE, sig_pipe_handler);

  kbot::Broker      broker{};
  kbot::ChannelPort channel_port;

  broker.run();

  for (;;)
  {
    const uint8_t mask = channel_port.Poll();

    if (kbot::HasRequest(mask))
    {
      kbot::log("In: REQUEST");
      channel_port.ReceiveIPCMessage(false);
      channel_port.SendIPCMessage(std::make_unique<okay_message>(), false);
    }

    if (kbot::HasReply(mask))
    {
      kbot::log("In: REPLY");
      channel_port.ReceiveIPCMessage(true);
    }

    if (broker.Poll() && channel_port.REQReady())
    {
      kbot::log("Out: REQUEST");
      channel_port.SendIPCMessage(std::move(broker.DeQueue()), true);
    }

    for (auto&& message : channel_port.GetRXMessages())
      broker.ProcessMessage(std::move(message));


    std::mutex                   mtx{};
    std::condition_variable      condition{};
    std::unique_lock<std::mutex> lock{mtx};
    condition.wait_for(lock, std::chrono::milliseconds(300));
  }

  broker.Shutdown();

  return 0;
}
