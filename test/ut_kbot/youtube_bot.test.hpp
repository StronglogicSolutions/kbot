#ifndef __YOUTUBE_TEST_HPP__
#define __YOUTUBE_TEST_HPP__

#include <string>
#include <iostream>

#include "gtest/gtest.h"

#include "youtube/youtube.hpp"


class YouTubeBotTestFixture: public ::testing::Test {
 protected:
  YouTubeBot bot;

  void SetUp() override {

  }

  void TearDown() override {

  }

 public:
  YouTubeBotTestFixture()
  : bot(YouTubeBot{}) {}
};

#endif // __YOUTUBE_TEST_HPP__
