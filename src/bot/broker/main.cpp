#include "broker.hpp"

int main(int argc, char** argv)
{
  kbot::Broker broker{};

  broker.run();

  for (uint8_t i{0}; i < 5; i++)
  {
    if (i == 1)
      broker.SendEvent(kbot::Platform::mastodon, "find comments");
    else
    if (i == 3)
      broker.SendEvent(kbot::Platform::youtube, "find livestream");

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  broker.Shutdown();

  return 0;
}
