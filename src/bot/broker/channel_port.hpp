#pragma once

#include <zmq.hpp>
#include "util/util.hpp"
#include "ipc.hpp"

namespace kbot {
const std::string DATA_REQUEST{"Get Results"};
const std::string TX_ADDR{"tcp://localhost:28474"};
const std::string RX_ADDR{"tcp://0.0.0.0:28473"};
const uint8_t     MAX_RETRIES{10};

static const bool HasIncomingRequest(uint8_t mask)
{
  return (mask & 0x01 << 1);
}

static const bool HasResponse(uint8_t mask)
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
m_tx{m_context, ZMQ_DEALER},
m_rx{m_context, ZMQ_ROUTER},
m_socket_num{1},
m_timeout   {50},
m_retries   {0}
{
  m_tx.set(zmq::sockopt::linger, 0);
  m_rx.set(zmq::sockopt::linger, 0);
  m_tx.set(zmq::sockopt::routing_id, "botbroker");
  m_rx.set(zmq::sockopt::routing_id, "botrouter");
  m_tx.connect(TX_ADDR);
  m_rx.bind(RX_ADDR);
}

bool ReceiveIPCMessage(const bool is_request = true)
{
  auto& socket = (is_request) ? m_rx : m_tx;
  zmq::message_t  identity;
  socket.recv(&identity);

  if (identity.empty())
    return false;

  std::vector<ipc_message::byte_buffer> received_message{};
  zmq::message_t                        message;
  int                                   more_flag{1};

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

  if (ipc_message)
  {
    m_rx_msgs.emplace_back(std::move(ipc_message));
    return true;
  }

  return false;
}

bool SendIPCMessage(u_ipc_msg_ptr message, const bool use_req = false)
{
  auto&          socket    = (use_req) ? m_tx : m_rx;
  auto           payload   = message->data();
  int32_t        frame_num = payload.size();

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
  uint8_t poll_mask{0x00};
  zmq::pollitem_t items[] = { { m_tx, 0, ZMQ_POLLIN, 0},
                              { m_rx, 1, ZMQ_POLLIN, 0} };
  zmq::poll(&items[0], 2, m_timeout);

  if (items[0].revents & ZMQ_POLLIN)
    poll_mask |= (0x01 << 0);
  if (items[1].revents & ZMQ_POLLIN)
    poll_mask |= (0x01 << 1);
  return poll_mask;
}

std::vector<u_ipc_msg_ptr> GetRXMessages()
{
  return std::move(m_rx_msgs);
}

bool REQReady()
{
  return m_tx_ready;
}

private:
static std::string const FindDataRequest(std::string message) {
  return (message.find(DATA_REQUEST) != std::string::npos) ?
    DATA_REQUEST :
    "";
}

zmq::context_t m_context;
zmq::socket_t  m_rx;
zmq::socket_t  m_tx;
std::vector<u_ipc_msg_ptr>     m_tx_msgs;
std::vector<u_ipc_msg_ptr>     m_rx_msgs;
uint8_t        m_socket_num;
uint8_t        m_timeout;
uint8_t        m_retries;
bool           m_tx_ready;

};
} // ns kbot
