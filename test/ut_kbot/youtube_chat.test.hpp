#ifndef __YOUTUBE_CHAT_TEST_HPP__
#define __YOUTUBE_CHAT_TEST_HPP__

#include <string>
#include <iostream>

#include "gtest/gtest.h"

#include "api/youtube/youtube_api.hpp"


class YouTubeChatTestFixture: public ::testing::Test {
 protected:
  youtube::YouTubeDataAPI api;

  void SetUp() override {

  }

  void TearDown() override {

  }

 public:
  YouTubeChatTestFixture()
  : api(youtube::YouTubeDataAPI{}) {}
};

#endif // __YOUTUBE_CHAT_TEST_HPP__
