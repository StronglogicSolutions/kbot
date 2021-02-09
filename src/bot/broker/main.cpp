#include "broker.hpp"
#include "channel_port.hpp"

const std::string OK{"OK"};

int main(int argc, char** argv)
{
  kbot::Broker      broker{};
  kbot::ChannelPort channel_port;

  broker.run();

  for (;;)
  {
    if (channel_port.Poll())
    {
      broker.ProcessMessage(channel_port.ReceiveMessage());
      channel_port.SendMessage(OK);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  broker.Shutdown();

  return 0;
}
