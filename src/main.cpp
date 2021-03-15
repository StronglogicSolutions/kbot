#include <iostream>
#include "bot/youtube/youtube.hpp"

static kbot::BotEvent ParseRuntimeArguments(int argc, char** argv) {
  kbot::BotEvent event{};

  for (int i = 1; i < argc; i++) {
    std::string argument = argv[i];

    if (argument.find("--message") == 0) {
      event.data = argument.substr(10);
      continue;
    }
    else
    if (argument.find("--keyword") == 0) {
      event.urls.emplace_back(argument.substr(10));
      continue;
    }
    else
    if (argument.find("--event") == 0) {
      event.name = argument.substr(8);
      continue;
    }
  }

  return event;
}


int main(int argc, char** argv) {
  using namespace kbot;
  auto event = ParseRuntimeArguments(argc, argv);
  YouTubeBot bot{};

  if (bot.HandleEvent(event))
  {
    kbot::log("Request handled successfully");
    return 0;
  }

  kbot::log("Request failed");

  return 1;
}
