#include "kbot.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

TEST(KBotTest, YoutubeBotInstantiated) {
  ASSERT_NO_THROW(YoutubeBot{});
}

TEST_F(KBotTestFixture, YouTubeBotRuns) {
  ASSERT_NO_THROW(bot.getName());
}

TEST_F(KBotTestFixture, YouTubeThreadStarts) {
  ASSERT_NO_THROW(bot.start());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  bot.stop();
  EXPECT_TRUE((bot.m_loops > 0));
}
