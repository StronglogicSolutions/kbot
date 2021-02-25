#include <iostream>
#include "bot/youtube/youtube.hpp"
#include <zmq.hpp>

const std::string DATA_REQUEST{"Get Results"};

class ChannelPort
{
public:
ChannelPort()
:
m_context   {1},
m_socket    {m_context, ZMQ_REP},
m_socket_num{1},
m_timeout   {0}
{
  Reset();
}

void Reset()
{
  m_socket.bind("tcp://0.0.0.0:28473");
}

std::string ReceiveMessage() {
  zmq::message_t message{};

  zmq::recv_result_t result = m_socket.recv(message, zmq::recv_flags::none);

  if (result.has_value()) {
    return std::string{
      static_cast<char*>(message.data()),
      static_cast<char*>(message.data()) + message.size()
    };
  }

  return "";
}

bool SendMessage(std::string message) {
  kbot::log("Sending IPC message: " + message + "\n");
  zmq::message_t ipc_msg{message.size()};
  memcpy(ipc_msg.data(), message.data(), message.size());
  zmq::send_result_t result = m_socket.send(std::move(ipc_msg), zmq::send_flags::none);

  return result.has_value();
}

bool Poll()
{
  void*          socket_ptr = static_cast<void*>(m_socket);
  zmq_pollitem_t items[1]{
    {socket_ptr, 0, ZMQ_POLLIN, 0}
  };

  zmq::poll(&items[0], m_socket_num, m_timeout);

  return (items[0].revents & ZMQ_POLLIN);

}

static bool IsDataRequest(std::string s) {
  return (FindDataRequest(s) == DATA_REQUEST);
}

private:
static std::string const FindDataRequest(std::string message) {
  if (message.find(DATA_REQUEST) != std::string::npos) {
    return DATA_REQUEST;
  }
  return "";
}

zmq::context_t m_context;
zmq::socket_t  m_socket;
uint8_t        m_socket_num;
uint8_t        m_timeout;
};


int main(int argc, char** argv) {
  using namespace kbot;

  ChannelPort channel_port{};

  try {
    YouTubeBot bot{};

    if (bot.init()) {
      bot.start();
    }


    for (;;) {
      if (channel_port.Poll())
      {
        std::string message = channel_port.ReceiveMessage();
        kbot::log("Received IPC message: \n" + message);

        if (ChannelPort::IsDataRequest(message))
        {
          if (!channel_port.SendMessage(bot.GetResults())) {
            kbot::log("Failed to send response");
          }
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    bot.stop();
  }
  catch (const std::exception& e) {
    kbot::log(e.what());
  }

  return 0;
}
