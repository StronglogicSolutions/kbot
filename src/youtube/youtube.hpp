#ifndef __YOUTUBE_HPP__
#define __YOUTUBE_HPP__

#include <iostream>
#include <interfaces/interfaces.hpp>
#include <chrono>

class YoutubeBot : public Bot, public Worker {
 public:
  YoutubeBot()
  : Bot("YoutubeBot") {}

  virtual void loop() override {
    while (m_is_running) {
      m_loops++;
      std::cout << "YouTube bot loop" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }
};

#endif // __YOUTUBE_HPP__
