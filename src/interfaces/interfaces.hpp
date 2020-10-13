#ifndef __INTERFACES_HPP__
#define __INTERFACES_HPP__

#include <string>
#include <thread>
#include <memory>

class API {
 public:
  virtual std::string GetType() = 0;
  virtual bool        TestMode() {
    return true;
  }
};

class Bot {
 public:
  Bot(std::string name)
  : m_name(name) {}

  virtual ~Bot() {}
  std::string GetName() { return m_name; }

  virtual std::unique_ptr<API> GetAPI(std::string name) = 0;

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

#endif // __INTERFACES_HPP__