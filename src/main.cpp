#include <iostream>
#include "bot/youtube/youtube.hpp"
#include <zmq.hpp>

const std::string DATA_REQUEST{"Get Results"};

std::string ReceiveMessage(zmq::socket_t* socket) {
  zmq::message_t message{};

  zmq::recv_result_t result = socket->recv(message, zmq::recv_flags::none);

  if (result.has_value()) {
    return std::string{
      static_cast<char*>(message.data()),
      static_cast<char*>(message.data()) + message.size()
    };
  }

  return "";
}

std::string const ProcessMessage(std::string message) {
  if (message.find(DATA_REQUEST) != std::string::npos) {
    return DATA_REQUEST;
  }
  return "";
}

bool SendMessage(zmq::socket_t* socket, std::string message) {
  log("Sending IPC message: " + message + "\n");
  zmq::message_t ipc_msg{message.size()};
  memcpy(ipc_msg.data(), message.data(), message.size());
  zmq::send_result_t result = socket->send(std::move(ipc_msg), zmq::send_flags::none);

  return result.has_value();
}

bool IsDataRequest(std::string s) {
  return s.compare(DATA_REQUEST) == 0;
}

using namespace kbot;

int main(int argc, char** argv) {
  zmq::context_t context   {1};
  zmq::socket_t  socket    {context, ZMQ_REP};
  uint8_t        socket_num{1};
  uint8_t        timeout   {0};

  try {
    YouTubeBot bot{};

    if (bot.init()) {
      bot.start();
    }

    socket.bind("tcp://0.0.0.0:28473");

    void* socket_ptr = static_cast<void*>(socket);

    for (;;) {
      zmq_pollitem_t items[1]{
        {socket_ptr, 0, ZMQ_POLLIN, 0}
      };

      zmq::poll(&items[0], socket_num, timeout);

      if (
        items[0].revents & ZMQ_POLLIN &&                       // Message
        IsDataRequest(ProcessMessage(ReceiveMessage(&socket))) // Request confirmed
      ) {
        log ("Received IPC message");

        if (!SendMessage(&socket, bot.GetResults())) {
          log("Failed to send response");
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    bot.stop();
  }
  catch (const std::exception& e) {
    log(e.what());
  }

  return 0;
}
