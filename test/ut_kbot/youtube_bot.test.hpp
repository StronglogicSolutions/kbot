#ifndef __YOUTUBE_TEST_HPP__
#define __YOUTUBE_TEST_HPP__

#include <string>
#include <iostream>

#include "gtest/gtest.h"

#include "bot/youtube/youtube.hpp"


class YouTubeBotTestFixture: public ::testing::Test {
 protected:
  kiq::kbot::YouTubeBot bot;

  void SetUp() override {

  }

  void TearDown() override {

  }

 public:
  YouTubeBotTestFixture()
  : bot(kiq::kbot::YouTubeBot{}) {}
};

#endif // __YOUTUBE_TEST_HPP__
