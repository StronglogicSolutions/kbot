#include "youtube_chat.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

std::string TEST_AUTHOR_1{"Jazilla"};
std::string TEST_AUTHOR_2{"Jizzimiah"};

/**
 * BotInstantiated
 */
TEST_F(YouTubeChatTestFixture, CreateChatMap) {
  youtube::VideoDetails video_details{
    .id      = "Random ID",
    .chat_id = TEST_AUTHOR_1
  };

  youtube::LiveMessages messages{};

  messages.reserve(15);

  messages.emplace_back(
    std::move(youtube::LiveMessage{
      .timestamp = "2020-10-26T12:00:00",
      .author = TEST_AUTHOR_1,
      .text   = "Fantastic. Nice to meet you, @Emmanuel Buckshi!",
      .tokens = std::vector<conversation::Token>{}
    })
  );
  youtube::LiveChatMap chat_map{};

  chat_map.insert({TEST_AUTHOR_1, messages});

  api.SetVideoDetails(video_details);
  api.SetChatMap(chat_map);

  api.ParseTokens();

  youtube::LiveMessages mentions = api.FindMentions();

  youtube::LiveChatMap parsed_chats = api.GetChats();

  for (const auto& chat : parsed_chats) {
    auto messages = chat.second;

    for (const auto& message : messages) {
      if (!message.tokens.empty()) {
        for (const auto& token : message.tokens) {
          std::cout << "Token being parsed: " << token.value << std::endl;
        }
      }
    }
  }

  for (const auto& mention : mentions) {
    std::cout << mention.author << " says: " << mention.text << std::endl;
  }

  EXPECT_EQ(true, true);
}
