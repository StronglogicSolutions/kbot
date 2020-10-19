#include <iostream>
#include "bot/youtube/youtube.hpp"

int main(int argc, char** argv) {
  try {
    YouTubeBot bot{};

    if (bot.init()) {
      bot.start();
    }

    // for (uint8_t i = 0; i < 5; i++) {
    //   std::this_thread::sleep_for(std::chrono::seconds(30));
    // }

    for (;;)
      ;

    bot.stop();
  }
  catch (const std::exception& e) {
    log(e.what());
  }

  return 0;
}
