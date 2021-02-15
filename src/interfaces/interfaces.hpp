#pragma once

#include <string>
#include <thread>
#include <memory>
#include <vector>

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
  discord  = 0x02
};

struct BotEvent
{
Platform    platform;
std::string name;
std::string data;
std::vector<std::string> urls;
};

using BrokerCallback = bool(*)(BotEvent event);

class Bot {
 public:
  Bot(std::string name)
  : m_name(name) {}

  virtual ~Bot() {}
  std::string GetName() { return m_name; }

  virtual std::unique_ptr<API> GetAPI(std::string name) = 0;
  virtual void                 SetCallback(BrokerCallback cb_fn_ptr) = 0;
  virtual bool                 HandleEvent(BotEvent event) = 0;
  virtual bool                 IsRunning() = 0;
  virtual void                 Start() = 0;
  virtual void                 Shutdown() = 0;
  virtual void                 Init() = 0;

 private:
  std::string m_name;
};

class Worker {
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

} // namespace kbot
