#ifndef __KBOT_TEST_HPP__
#define __KBOT_TEST_HPP__

#include <string>
#include <iostream>

#include "gtest/gtest.h"

#include "youtube/youtube.hpp"


class KBotTestFixture: public ::testing::Test {
 protected:
  YoutubeBot bot;

  void SetUp() override {

  }

  void TearDown() override {

  }

 public:
  KBotTestFixture()
  : bot(YoutubeBot{}) {}
};

#endif // __KBOT_TEST_HPP__
