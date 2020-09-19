#include "youtube_bot.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

TEST_F(YoutubeBotTestFixture, BotInstantiated) {
  ASSERT_NO_THROW(YoutubeBot{});
}

TEST_F(YoutubeBotTestFixture, BotGetsName) {
  ASSERT_NO_THROW(bot.GetName());
}

TEST_F(YoutubeBotTestFixture, BotStartsAndStopsThread) {
  ASSERT_NO_THROW(bot.start());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  bot.stop();
  EXPECT_TRUE((bot.m_loops > 0));
}

TEST_F(YoutubeBotTestFixture, GetApiReturnsDefaultApi) {
  std::unique_ptr<API> api = bot.GetAPI();
  EXPECT_EQ(api->GetType(), "Default API");
}
