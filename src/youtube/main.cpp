#include <iostream>
#include "youtube/youtube.hpp"

int main(int argc, char** argv) {
  YouTubeBot bot{};

  std::cout << bot.GetName() << std::endl;

  std::unique_ptr<API> api = bot.GetAPI("YouTube Data API");

  static_cast<YouTubeDataAPI*>(api.get())->GetType();

  return 0;
}
