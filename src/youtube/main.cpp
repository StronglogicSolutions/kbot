#include <iostream>
#include "youtube/youtube.hpp"

int main(int argc, char** argv) {
  YoutubeBot bot{};

  std::cout << bot.GetName() << std::endl;

  return 0;
}
