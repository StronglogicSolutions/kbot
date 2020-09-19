#ifndef __YOUTUBE_TEST_HPP__
#define __YOUTUBE_TEST_HPP__

#include <string>
#include <iostream>

#include "gtest/gtest.h"

#include "youtube/youtube.hpp"


class YoutubeBotTestFixture: public ::testing::Test {
 protected:
  YoutubeBot bot;

  void SetUp() override {

  }

  void TearDown() override {

  }

 public:
  YoutubeBotTestFixture()
  : bot(YoutubeBot{}) {}
};

#endif // __YOUTUBE_TEST_HPP__
