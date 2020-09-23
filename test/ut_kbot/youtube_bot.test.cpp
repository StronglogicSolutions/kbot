#include "youtube_bot.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

/**
 * BotInstantiated
 */
TEST_F(YoutubeBotTestFixture, BotInstantiated) {
  ASSERT_NO_THROW(YouTubeBot{});
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
  YouTubeBot bot{};

  std::unique_ptr<API> request_api = bot.GetAPI("Request API");

  std::string result = static_cast<RequestAPI*>(request_api.get())->Get();

  std::cout << "RequestAPI returned: " << result << std::endl;

  EXPECT_FALSE(result.empty());
}

TEST_F(YoutubeBotTestFixture, YouTubeDataAPI) {
  std::unique_ptr<API> youtube_api = bot.GetAPI("YouTube Data API");

  json     auth_json = json::parse(static_cast<YouTubeDataAPI*>(youtube_api.get())->GetToken());
  AuthData auth_data = static_cast<YouTubeDataAPI*>(youtube_api.get())->GetAuth();

  EXPECT_EQ(auth_json["access_token"].dump(), auth_data.access_token);
  EXPECT_EQ(auth_json["scope"].dump(),        auth_data.scope);
  EXPECT_EQ(auth_json["token_type"].dump(),   auth_data.token_type);
  EXPECT_EQ(auth_json["expiry_date"].dump(),  auth_data.expiry_date);

  std::string channel_info = static_cast<YouTubeDataAPI*>(youtube_api.get())->GetChannelInfo();
  std::cout << channel_info << std::endl;

  EXPECT_EQ(channel_info.empty(), false);
}