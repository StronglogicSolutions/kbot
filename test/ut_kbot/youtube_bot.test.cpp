#include "youtube_bot.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

/**
 * BotInstantiated
 */
TEST_F(YouTubeBotTestFixture, BotInstantiated) {
  ASSERT_NO_THROW(youtube::YouTubeBot{});
}

/**
 * BotGetsName
 */
TEST_F(YouTubeBotTestFixture, BotGetsName) {
  ASSERT_NO_THROW(bot.GetName());
}

/**
 * BotStartsAndStopsThread
 */
TEST_F(YouTubeBotTestFixture, BotStartsAndStopsThread) {
  ASSERT_NO_THROW(bot.start());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  bot.stop();
  EXPECT_TRUE((bot.m_loops > 0));
}

/**
 * GetApiReturnsDefaultApi
 */
TEST_F(YouTubeBotTestFixture, GetAPIReturnsDefaultApi) {
  std::unique_ptr<API> api = bot.GetAPI();
  EXPECT_EQ(api->GetType(), "Default API");
}

TEST(YouTubeBotTest, RequestAPIPerformsGet) {
  youtube::YouTubeBot  bot{};
  std::unique_ptr<API> request_api = bot.GetAPI("Request API");
  std::string          result = static_cast<RequestAPI*>(request_api.get())->Get();

  EXPECT_FALSE(result.empty());
}

TEST_F(YouTubeBotTestFixture, DISABLED_YouTubeDataAPI) {
  using namespace youtube;
  std::unique_ptr<API> youtube_api = bot.GetAPI("YouTube Data API");

  json     auth_json = json::parse(static_cast<YouTubeDataAPI*>(youtube_api.get())->FetchToken());
  AuthData auth_data = static_cast<YouTubeDataAPI*>(youtube_api.get())->GetAuth();

  EXPECT_EQ(auth_json["access_token"].dump(), auth_data.access_token);
  EXPECT_EQ(auth_json["scope"].dump(),        auth_data.scope);
  EXPECT_EQ(auth_json["token_type"].dump(),   auth_data.token_type);
  EXPECT_EQ(auth_json["expiry_date"].dump(),  auth_data.expiry_date);
}

TEST_F(YouTubeBotTestFixture, LiveStreamFetchMessages) {
  using namespace youtube;

  std::unique_ptr<API> youtube_api = bot.GetAPI("YouTube Data API");

  std::string token_info = static_cast<YouTubeDataAPI*>(youtube_api.get())->FetchToken();
  std::string video_id   = static_cast<YouTubeDataAPI*>(youtube_api.get())->FetchLiveVideoID();
  bool        live_info  = static_cast<YouTubeDataAPI*>(youtube_api.get())->FetchLiveDetails();
  std::string messages   = static_cast<YouTubeDataAPI*>(youtube_api.get())->FetchChatMessages();

  EXPECT_FALSE(messages.empty());
}

/**
 * PostMessage
 */
TEST_F(YouTubeBotTestFixture, PostMessage) {
  using namespace youtube;
  std::unique_ptr<API> api    = bot.GetAPI("YouTube Data API");
  YouTubeDataAPI* youtube_api = static_cast<YouTubeDataAPI*>(api.get());

  std::string token_info       = youtube_api->FetchToken();
  std::string video_id         = youtube_api->FetchLiveVideoID();
  bool        fetched_details  = youtube_api->FetchLiveDetails();
  std::string messages         = youtube_api->FetchChatMessages();
  bool        result           = youtube_api->PostMessage("Hope you are all well");

  EXPECT_EQ(result, true);
}

/**
 * Loop
 */
TEST_F(YouTubeBotTestFixture, Loop) {
  using namespace youtube;
  size_t        count{};
  std::string   chat_id{};
  LiveMessages  messages{};

  if (bot.init()) {
    LiveChatMap chats = bot.GetChats();
    count = chats.size();
    for (const auto& c : chats) {
      chat_id = c.first;
      messages = c.second;

      for (const auto& message : messages) {
        std::cout << message.timestamp << " - " << message.author << ": " << message.text << std::endl;
      }
    }
    bot.PostMessage("Thankful for the discussion");
  }

  EXPECT_EQ(count > 0, true);
  EXPECT_EQ(chat_id.empty(), false);
  EXPECT_EQ(messages.size() > 0, true);

  bot.start();

  for (uint8_t i = 0; i < 5; i++) {
    std::this_thread::sleep_for(std::chrono::seconds(30));
  }

  bot.stop();
}

/**
 * ParsingStringFindsTokens
 */
TEST(YouTubeAPITest, ParsingStringFindsTokens) {
  using namespace youtube;
  std::string     s{"This is a string with tokens in it, created by [PERSON Emmanuel] while living in [LOCATION Canada]"};
  YouTubeBot      bot{};
  auto            api         = bot.GetAPI("YouTubeDataAPI");
  YouTubeDataAPI* youtube_api = static_cast<YouTubeDataAPI*>(api.get());

  std::vector<Token> tokens = SplitTokens(s);

  for (const auto& token : tokens) {
    std::cout << "Token type: "    << token.type
              << "\nToken value: " << token.value
              << std::endl;
  }

  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens.at(0).type, TokenType::person);
  EXPECT_EQ(tokens.at(0).value, "Emmanuel");
  EXPECT_EQ(tokens.at(1).type, TokenType::location);
  EXPECT_EQ(tokens.at(1).value, "Canada");
}
