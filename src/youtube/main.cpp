#include <iostream>
#include "youtube/youtube.hpp"

int main(int argc, char** argv) {
  YouTubeBot bot{};

  if (bot.init()) {
    bot.start();
  }

  for (uint8_t i = 0; i < 5; i++) {
    std::this_thread::sleep_for(std::chrono::seconds(30));
  }

  bot.stop();

  return 0;
}
