#include "youtube.hpp"

static void StripWrappingQuotes(std::string& s)
{
  if (!s.empty())
  {
    char front = s.front();
    char back = s.back();
    if (front == '\'' || front == '"')
      s.erase(s.begin());
    if (back == '\'' || back == '"')
      s.erase(s.end() - 1);
  }
}

static kbot::BotRequest ParseRuntimeArguments(int argc, char** argv) {
  kbot::BotRequest request{};

  for (int i = 1; i < argc; i++) {
    std::string argument = argv[i];

    if (argument.find("--message") == 0) {

      request.data = argument.substr(10);
      StripWrappingQuotes(request.data);
      continue;
    }
    else
    if (argument.find("--keyword") == 0) {
      request.urls.emplace_back(argument.substr(10));
      continue;
    }
    else
    if (argument.find("--event") == 0) {
      request.event = argument.substr(8);
      continue;
    }
    if (argument.find("--id") == 0) {
      request.id = argument.substr(5);
      continue;
    }
  }

  return request;
}


int main(int argc, char** argv) {
  using namespace kbot;
  auto event = ParseRuntimeArguments(argc, argv);
  YouTubeBot bot{};
  bot.Init();

  if (bot.HandleEvent(event))
  {
    kbot::log("Request handled successfully");
    return 0;
  }

  kbot::log("Request failed");

  return 1;
}
