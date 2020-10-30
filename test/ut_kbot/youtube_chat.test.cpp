#include "youtube_chat.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>


/**
 * BotInstantiated
 */
TEST_F(YouTubeChatTestFixture, CreateChatMap) {
  // Video details with Chat ID to associate messages
  api.SetVideoDetails(TEST_VIDEO_DETAILS);
  // Variables
  youtube::LiveMessages messages{};
  youtube::LiveChatMap  chat_map{};
  auto                  live_messages_1 = GetLiveMessageSet1();
  // Capacity
  messages.reserve(15);
  // Insert 5 messages
  messages.insert(
    messages.end(),
    std::make_move_iterator(live_messages_1.begin()),
    std::make_move_iterator(live_messages_1.end())
  );
  // Add messages to map
  chat_map.insert({TEST_CHAT_ID, messages});

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
      nlp.Insert(
        conversation::Message{
          .text = message.text,
          .received = message.author == TEST_USERNAME,
          .next = nullptr
        },
        message.author,
        message.tokens.empty() ? "Unkown" : message.tokens.front().value
      );
    }
  }

  std::cout << nlp.toString() << std::endl;

  EXPECT_FALSE(nlp.toString().empty());
}
