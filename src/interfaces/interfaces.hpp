#pragma once

#include <string>
#include <thread>
#include <memory>
#include <vector>

#include "util/util.hpp"

class API {
 public:
  virtual std::string GetType() = 0;
  virtual bool        TestMode() {
    return true;
  }
};

namespace kbot {
class  Broker;

enum Platform
{
  youtube  = 0x00,
  mastodon = 0x01,
  discord  = 0x02,
  blog     = 0x03,
  telegram = 0x04,
  matrix   = 0x05,
  unknown  = 0x06
};

static enum Platform get_platform(const std::string& name)
{
  if (name == "Discord")
    return Platform::discord;
  if (name == "Mastodon")
    return Platform::mastodon;
  if (name == "YouTube")
    return Platform::youtube;
  if (name == "Blog")
    return Platform::blog;
  if (name == "Telegram")
    return Platform::telegram;
  if (name == "Matrix")
    return Platform::matrix;
  return Platform::unknown;
}

static const std::string get_platform_name(Platform platform)
{
  if (platform == Platform::youtube)
    return "YouTube";
  if (platform == Platform::mastodon)
    return "Mastodon";
  if (platform == Platform::discord)
    return "Discord";
  if (platform == Platform::blog)
    return "Blog";
  if (platform == Platform::telegram)
    return "Telegram";
  if (platform == Platform::matrix)
    return "Matrix";

  return "";
};

struct BotRequest
{
Platform                 platform;
std::string              event;
std::string              username;
std::string              data;
std::string              args;
std::vector<std::string> urls;
std::string              id;
std::string              previous_event;
uint32_t                 cmd{0x00};

const std::string to_string() const
{
  auto text = (data.size() > 30) ? data.substr(0, 30) : data;
  return "Platform:       " + get_platform_name(platform) + '\n' +
         "Event:          " + event + '\n' +
         "Username:       " + username + '\n' +
         "Data:           " + text + '\n' +
         "Urls:           " + url_string() + '\n' +
         "Id:             " + id + '\n' +
         "Previous_event: " + previous_event + '\n' +
         "Args:           " + args + '\n' +
         "Cmd:            " + std::to_string(cmd) + '\n';
}

const std::string url_string() const
{
  std::string output;
  std::string delim{};

  for (const auto& url : urls)
  {
    output += delim + url;
    delim = ">";
  }

  return output;
}

static const std::vector<std::string> urls_from_string(std::string input_string)
{
  std::string s = kbot::UnescapeQuotes(input_string);
  static const std::string delim{'>'};
  std::vector<std::string> urls{};

  if (!s.empty())
  {
    auto pos = s.find_first_of(delim);

    while (pos != s.npos)
    {
      urls.emplace_back(s.substr(0, pos));
      s = s.substr(pos + 1);
      pos = s.find_first_of(delim);
    }

    urls.emplace_back(s);
  }
  return urls;
}
};

static const bool SHOULD_REPOST{true};
static const bool SHOULD_NOT_REPOST{false};
static const std::string SUCCESS_EVENT{"bot:success"};
static const std::string INFO_EVENT   {"bot:info"};

static const BotRequest CreateSuccessEvent(const BotRequest& previous_event)
{
  kbot::log("Creating success event");
  return BotRequest{
    .platform       = previous_event.platform,
    .event          = SUCCESS_EVENT,
    .username       = previous_event.username,
    .data           = previous_event.data,
    .args           = previous_event.args,
    .urls           = previous_event.urls,
    .id             = previous_event.id,
    .previous_event = previous_event.event
  };
}

static const BotRequest CreateInfo(const std::string& info, const std::string& type, const BotRequest& previous_event)
{
  kbot::log("Creating info event");
  return BotRequest{
    .platform       = previous_event.platform,
    .event          = INFO_EVENT,
    .username       = previous_event.username,
    .data           = info,
    .args           = type,
    .urls           = previous_event.urls,
    .id             = previous_event.id,
    .previous_event = previous_event.event
  };
}

static const BotRequest CreateRequest(const std::string& message, const std::string& args, const BotRequest& previous_event)
{
  kbot::log("Creating request with message ", message.c_str(), " and args ", args.c_str());
  return BotRequest{
    .platform       = previous_event.platform,
    .event          = "bot:request",
    .username       = previous_event.username,
    .data           = message,
    .args           = args,
    .id             = previous_event.id,
    .previous_event = previous_event.event
  };
}

static const BotRequest CreateErrorEvent(const std::string& error_message, const BotRequest& previous_event)
{
  kbot::log("Creating error event");
  return BotRequest{
    .platform = previous_event.platform,
    .event    = "bot:error",
    .username = previous_event.username,
    .data     = error_message,
    .urls     = previous_event.urls,
    .id       = previous_event.id
  };
}

using BrokerCallback = bool(*)(BotRequest event);

class Bot
{
public:
Bot(std::string name)
: m_name(name) {}

virtual ~Bot() {}
std::string GetName() { return m_name; }

virtual std::unique_ptr<API> GetAPI(std::string name) = 0;
virtual void                 SetCallback(BrokerCallback cb_fn_ptr) = 0;
virtual bool                 HandleEvent(BotRequest event) = 0;
virtual bool                 IsRunning() = 0;
virtual void                 Start() = 0;
virtual void                 Shutdown() = 0;
virtual void                 Init() = 0;

private:
std::string m_name;
};

class Worker
{
public:
Worker()
: m_is_running(false) {}

virtual void start() {
  m_is_running = true;
  m_thread = std::thread(Worker::run, this);
}

static void run(void* worker) {
  static_cast<Worker*>(worker)->loop();
}

void stop() {
  m_is_running = false;
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

bool        m_is_running;
uint32_t    m_loops;

protected:
virtual void loop() = 0;

private:
std::thread m_thread;
};

struct PlatformQuery
{
Platform                 platform;
std::string              subject;
std::string              text;
std::vector<std::string> urls;
};


} // namespace kbot
