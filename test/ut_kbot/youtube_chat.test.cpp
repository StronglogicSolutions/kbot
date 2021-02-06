#include "youtube_chat.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

std::string MockTranslate(std::string s) {
  return "Translated text";
}

std::string CreateReply(std::string                      message,
                        conversation::SubjectiveContext* sub_ctx,
                        conversation::ObjectiveContext*  obj_ctx) {
  std::string response_text{};
  const auto  subject     = sub_ctx->Current();
  bool        is_question = obj_ctx->is_question;
  bool        is_continue = obj_ctx->is_continuing;

  if (is_question) {
    if (obj_ctx->question_type = conversation::QuestionType::TRANSLATE) {
       response_text = MockTranslate(message);
    }
  } else {
    response_text += "Sup, bitch?";
  }
  return response_text;
}

/**
 * DetectQuestionType
 *
 * TODO: Move to NLP ut
 */
TEST(YouTubeTranslateTest, DetectQuestionType) {
  std::string question_text{"Hey where are you?"};

  auto question_type = conversation::DetectQuestionType(question_text);

  EXPECT_EQ(question_type, conversation::QuestionType::WHERE);
}

/**
 * BotInstantiated
 */
TEST_F(YouTubeChatTestFixture, CreateChatMap) {
  // Video details with Chat ID to associate messages
  api.SetVideoDetails(TEST_VIDEO_DETAILS);
  // Variables
  kbot::LiveMessages messages{};
  kbot::LiveChatMap  chat_map{};
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

  kbot::LiveMessages mentions = api.FindMentions();

  kbot::LiveChatMap parsed_chats = api.GetChats();

  std::vector<std::string> reply_messages{};

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
          .received = message.author != TEST_USERNAME,
          .next = nullptr
        },
        message.author,
        message.tokens.empty() ? "Unknown" : message.tokens.front().value
      );
    }
  }

  // Reply
  for (const auto& conv : nlp.GetConversations()) {
    std::string reply;
    conversation::Message* node = conv.second;
    do {
      conversation::Message* node_next = node->next;
      reply = CreateReply(node->text, node->subjective, node->objective);
      if (node->received) {
        nlp.Reply(node, reply, TEST_USERNAME);
      }
      node = node_next;
    }
    while (node != nullptr);
  }









  // std::cout << "Second set" << std::endl;

  // youtube::LiveMessages second_message_sequence = GetLiveMessageSet2();

  // api.InsertMessages(TEST_CHAT_ID, second_message_sequence);
  // api.ParseTokens();

  // mentions = api.FindMentions();

  // parsed_chats = api.GetChats();

  // for (const auto& chat : parsed_chats) {
  //   auto messages = chat.second;

  //   for (const auto& message : messages) {
  //     if (!message.tokens.empty()) {
  //       for (const auto& token : message.tokens) {
  //         std::cout << "Token being parsed: " << token.value << std::endl;
  //       }
  //     }
  //     nlp.Insert(
  //       conversation::Message{
  //         .text = message.text,
  //         .received = message.author == TEST_USERNAME,
  //         .next = nullptr
  //       },
  //       message.author,
  //       message.tokens.empty() ? "Unknown" : message.tokens.front().value
  //     );
  //   }
  // }
  std::cout << nlp.toString() << std::endl;

}
