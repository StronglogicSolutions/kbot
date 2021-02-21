#pragma once

#include <zmq.hpp>
#include "util/util.hpp"
#include "ipc.hpp"

namespace kbot {
const std::string DATA_REQUEST{"Get Results"};
const std::string REP_ADDRESS{"tcp://0.0.0.0:28473"};
const std::string REQ_ADDRESS{"tcp://0.0.0.0:28474"};

static const bool HasReply(uint8_t mask)
{
  return (mask & 0x01 << 1);
}

static const bool HasRequest(uint8_t mask)
{
  return (mask & 0x01 << 0);
}

class ChannelPort
{
public:
using u_ipc_msg_ptr = ipc_message::u_ipc_msg_ptr;
ChannelPort()
:
m_context   {1},
m_rep_socket{m_context, ZMQ_REP},
m_req_socket{m_context, ZMQ_REQ},
m_socket_num{2},
m_timeout   {0}
{
  Reset();
}

void Reset()
{
  m_rep_socket.bind(REP_ADDRESS);
  m_req_socket.connect(REQ_ADDRESS);
}

std::string ReceiveMessage() {
  zmq::message_t     message{};
  zmq::recv_result_t result = m_rep_socket.recv(message, zmq::recv_flags::none);

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
  zmq::send_result_t result = m_rep_socket.send(std::move(ipc_msg), zmq::send_flags::none);

  return result.has_value();
}

bool ReceiveIPCMessage(const bool use_req = true)
{
  std::vector<ipc_message::byte_buffer> received_message{};
  zmq::message_t                        message;
  int                                   more_flag{1};

  zmq::socket_t&                        socket = (use_req) ?
                                                   m_req_socket :
                                                   m_rep_socket;

  while (more_flag)
  {
    socket.recv(&message, static_cast<int>(zmq::recv_flags::none));
    size_t size = sizeof(more_flag);
    socket.getsockopt(ZMQ_RCVMORE, &more_flag, &size);

    received_message.push_back(std::vector<unsigned char>{
        static_cast<char*>(message.data()), static_cast<char*>(message.data()) + message.size()
    });
  }

  ipc_message::u_ipc_msg_ptr ipc_message = DeserializeIPCMessage(std::move(received_message));

  if (ipc_message != nullptr)
  {
    m_rx_msgs.emplace_back(std::move(ipc_message));
    return true;
  }

  return false;
}

bool SendIPCMessage(u_ipc_msg_ptr message, const bool use_req = false)
{
  auto           payload   = message->data();
  int32_t        frame_num = payload.size();
  zmq::socket_t& socket    = (use_req) ?
                               m_req_socket :
                               m_rep_socket;

  for (int i = 0; i < frame_num; i++)
  {
    int  flag  = i == (frame_num - 1) ? 0 : ZMQ_SNDMORE;
    auto data  = payload.at(i);

    zmq::message_t message{data.size()};
    std::memcpy(message.data(), data.data(), data.size());

    socket.send(message, flag);
  }

  return true;
}

uint8_t Poll()
{
  uint8_t        poll_mask{0x00};
  void*          rep_socket_ptr = static_cast<void*>(m_rep_socket);
  void*          req_socket_ptr = static_cast<void*>(m_req_socket);
  zmq_pollitem_t items[2]{
    {rep_socket_ptr, 0, ZMQ_POLLIN, 0},
    {req_socket_ptr, 0, ZMQ_POLLIN, 0}
  };

  zmq::poll(&items[0], m_socket_num, m_timeout);

  if (items[0].revents & ZMQ_POLLIN)
    poll_mask |= (0x01 << 0);

  if (items[1].revents & ZMQ_POLLIN)
    poll_mask |= (0x01 << 1);

  return poll_mask;
}

static bool IsDataRequest(std::string s) {
  return (FindDataRequest(s) == DATA_REQUEST);
}

std::vector<u_ipc_msg_ptr> GetRXMessages()
{
  return std::move(m_rx_msgs);
}

private:
static std::string const FindDataRequest(std::string message) {
  return (message.find(DATA_REQUEST) != std::string::npos) ?
    DATA_REQUEST :
    "";
}

zmq::context_t m_context;
zmq::socket_t  m_rep_socket;
zmq::socket_t  m_req_socket;
std::vector<u_ipc_msg_ptr>     m_tx_msgs;
std::vector<u_ipc_msg_ptr>     m_rx_msgs;
uint8_t        m_socket_num;
uint8_t        m_timeout;
};
} // namespace kbot
