#pragma once

#include <zmq.hpp>
#include "util/util.hpp"

namespace kbot {
const std::string DATA_REQUEST{"Get Results"};
const std::string LOCAL_ADDRESS{"tcp://0.0.0.0:28473"};

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
  m_socket.bind(LOCAL_ADDRESS);
}

std::string ReceiveMessage() {
  zmq::message_t     message{};
  zmq::recv_result_t result = m_socket.recv(message, zmq::recv_flags::none);

  return (result.has_value()) ?
    std::string{
      static_cast<char*>(message.data()),
      static_cast<char*>(message.data()) + message.size()
    } :
    "";
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
  return (message.find(DATA_REQUEST) != std::string::npos) ?
    DATA_REQUEST :
    "";
}

zmq::context_t m_context;
zmq::socket_t  m_socket;
uint8_t        m_socket_num;
uint8_t        m_timeout;
};
} // namespace kbot
