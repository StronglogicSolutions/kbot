#ifndef __YOUTUBE_HPP__
#define __YOUTUBE_HPP__

#include <iostream>
#include <interfaces/interfaces.hpp>

class YoutubeBot : public Bot {
 public:
  virtual void run() override {
    std::cout << "Youtube Bot is running" << std::endl;
  }
};

#endif // __YOUTUBE_HPP__
