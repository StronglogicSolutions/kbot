#include "kbot.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

TEST(KBotTest, YoutubeBotInstantiated) {
  ASSERT_NO_THROW(YoutubeBot{});
}

TEST_F(KBotTestFixture, YouTubeBotRuns) {
  ASSERT_NO_THROW(bot.run());
}
