#include "youtube_bot.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

/**
 * BotInstantiated
 */
TEST_F(YoutubeBotTestFixture, BotInstantiated) {
  ASSERT_NO_THROW(YoutubeBot{});
}

/**
 * BotGetsName
 */
TEST_F(YoutubeBotTestFixture, BotGetsName) {
  ASSERT_NO_THROW(bot.GetName());
}

/**
 * BotStartsAndStopsThread
 */
TEST_F(YoutubeBotTestFixture, BotStartsAndStopsThread) {
  ASSERT_NO_THROW(bot.start());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  bot.stop();
  EXPECT_TRUE((bot.m_loops > 0));
}

/**
 * GetApiReturnsDefaultApi
 */
TEST_F(YoutubeBotTestFixture, GetAPIReturnsDefaultApi) {
  std::unique_ptr<API> api = bot.GetAPI();
  EXPECT_EQ(api->GetType(), "Default API");
}

TEST(YoutubeBotTest, RequestAPIPerformsGet) {
  YoutubeBot bot{};

  std::unique_ptr<API> request_api = bot.GetAPI("Request API");

  std::string result = static_cast<RequestAPI*>(request_api.get())->Get();

  std::cout << "RequestAPI returned: " << result << std::endl;

  EXPECT_FALSE(result.empty());
}