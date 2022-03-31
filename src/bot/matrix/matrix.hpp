#pragma once

#include "katrix.hpp"
#include "interfaces/interfaces.hpp"

namespace kbot {
namespace katrix {
namespace constants {
static const uint8_t     APP_NAME_LENGTH{6};
static const std::string USER{};
static const char*       DEFAULT_CONFIG_PATH{"config/config.ini"};
static const char*       REQUEST_PollStop   {"poll stop"};
static const char*       REQUEST_PollResult {"poll result"};
static const char*       REQUEST_Rooms      {"process rooms"};
} // namespace constants

static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::APP_NAME_LENGTH  + 1));
}

static std::vector<std::string> GetArgs(const std::string& s)
{
  using json = nlohmann::json;
  json d = json::parse(s, nullptr, false);

  if (!d.is_null() && d.contains("args")) {
    return d["args"].get<std::vector<std::string>>();
  }
  return {};
}
} // ns katrix

class MatrixBot : public kbot::Worker,
                  public kbot::Bot,
                  public ::KatrixBot
{
public:
MatrixBot()
: kbot::Bot{"matrixuser"},
  ::KatrixBot{"matrix.org"}
{}

virtual void Init() override
{
  return;
}

virtual void loop() override
{

}

void SetCallback(BrokerCallback cb_fn)
{
  m_send_event_fn = cb_fn;
}

enum class TGCommand
{
message   = 0x00,
poll      = 0x01,
poll_stop = 0x02
};

bool HandleEvent(BotRequest request)
{
  return false;
}

virtual std::unique_ptr<API> GetAPI(std::string name) override
{
  return nullptr;
}

virtual bool IsRunning() override
{
  return m_is_running;
}

virtual void Start() override
{
  if (!m_is_running)
    Worker::start();
}

virtual void Shutdown() override
{
  Worker::stop();
}

private:
BrokerCallback m_send_event_fn;

};
} // namespace kbot
