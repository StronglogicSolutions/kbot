#ifndef __YOUTUBE_HPP__
#define __YOUTUBE_HPP__

#include <iostream>
#include <interfaces/interfaces.hpp>
#include <api/api.hpp>
#include <chrono>

/**
 * YouTubeBot
 *
 * @interface {Bot}
 *
 * Specializes in using the Google YouTube Data API
 *
 */
class YoutubeBot : public Bot, public Worker {
 public:
 /**
  * @constructor
  */
  YoutubeBot()
  : Bot("YouTubeBot") {}

  /**
   * loop
   *
   * The loop method runs on its own thread
   */
  virtual void loop() override {
    while (m_is_running) {
      m_loops++;
      std::cout << "YouTube bot loop" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  /**
   * GetAPI
   *
   * @param
   * @returns
   */
  virtual std::unique_ptr<API> GetAPI(std::string name = "") {
    if (name.empty()) {
      return std::make_unique<DefaultAPI>();
    }
    else
    if (name.compare("Request API")) {
      return std::make_unique<RequestAPI>();
    }
    return nullptr;
  }

};

#endif // __YOUTUBE_HPP__
