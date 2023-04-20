#include "broker.hpp"
#include "channel_port.hpp"
#include <condition_variable>
#include <mutex>

static void sig_pipe_handler(int signal)
{
  static const auto path = kutils::GetCWD(6) + "/sig_caught.log";
  const std::string message{"Latest signal " + std::to_string(signal) + " at " + kutils::get_simple_datetime()};
  kutils::SaveToFile(message, path);
}
namespace kbot {
struct SocketState
{
static const uint32_t TX_MAX_MISSES = 100;

SocketState()
: broker([this]() { ResetChannel(); })
{
  broker.run();
  channel.SendIPCMessage(std::make_unique<keepalive>(), true);
}

void ResetChannel()
{
  kutils::log("ResetChannel was called. Broker isn't happy");
}

void Poll()
{
  uint8_t mask;
  {
    std::mutex                   mtx;
    std::unique_lock<std::mutex> lock{mtx};
    mask = channel.Poll();
  }

  broker_has_messages = broker.Poll();
  has_request         = HasIncomingRequest(mask);
  has_response        = HasResponse(mask);
}

void ReceiveResponse()
{
  channel.ReceiveIPCMessage(false);
  channel.SendIPCMessage(std::make_unique<okay_message>(), false);
}

void ChannelSend()
{
  channel.SendIPCMessage(std::move(broker.DeQueue()), true);
}

void ProcessChannel()
{
  if (has_response)
    ReceiveResponse();
  if (has_request)
    ReceiveRequest();
}

void ProcessRX()
{
  for (auto&& message : channel.GetRXMessages())
    broker.ProcessMessage(std::move(message));
}

void ReceiveRequest()
{
  channel.ReceiveIPCMessage(true);
}

void Shutdown()
{
  broker.Shutdown();
}

void Transmit()
{
  if (broker_has_messages)
    ChannelSend();
}

ChannelPort channel;
Broker      broker;
bool        has_response       {false};
bool        has_request        {false};
bool        broker_has_messages{false};
bool        port_has_messages  {false};
bool        channel_can_send   {true};
bool        should_reset_tx    {false};
uint32_t    tx_misses          {0};
};
} // ks kbot

int main(int argc, char** argv)
{
  signal(SIGPIPE, sig_pipe_handler);
  kbot::SocketState state{};

  for (;;)
  {
    state.Poll();
    state.ProcessChannel();
    state.Transmit();
    state.ProcessRX();
  }

  state.Shutdown();

  return 0;
}
