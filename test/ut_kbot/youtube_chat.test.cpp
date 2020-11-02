#include "youtube_chat.test.hpp"
#include <chrono>
#include <cstdint>
#include <stdio.h>

namespace request {
void MakeRequest() {}
} // namespace request

namespace translation {
std::string ExtractSubtext(std::string text) {
 return "";
}

std::string TranslateText(std::string text) {
  // TODO: Replace no-op
  request::MakeRequest();
  return text;
}
} // namespace translation

std::string TranslateToKorean(std::string text) {
  return translation::TranslateText(
    translation::ExtractSubtext(
      text
    )
  );
}

conversation::QuestionType DetectQuestionType(std::string s) {
  uint8_t num = conversation::QTypeNames.size();
  for (uint8_t i = 2; i < num; i++) {
    if (s.find(conversation::QTypeNames.at(i)) != std::string::npos) {
      return static_cast<conversation::QuestionType>((i / 2));
    }
  }
  return conversation::QuestionType::UNKNOWN;
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
       response_text = TranslateToKorean(message);
    }
  }
  return response_text;
}

TEST(YouTubeTranslateTest, DetectQuestionType) {
  std::string question_text{"Hey where are you?"};

  auto question_type = DetectQuestionType(question_text);

  EXPECT_EQ(question_type, conversation::QuestionType::WHERE);
}

// TEST_F(YouTubeChatTestFixture, TranslateTest) {
//   json payload{};
//     payload["text"] = "I wish I could speak Korean";
//     payload["source"] = "en";
//     payload["target"] = "ko";

//     cpr::Response r = cpr::Post(
//       cpr::Url{"https://openapi.naver.com/v1/papago/n2mt"},
//       cpr::Header{
//         {"X-Naver-Client-Id", ""},
//         {"X-Naver-Client-Secret",   ""},
//         {"Content-Type", "application/x-www-form-urlencoded; charset=UTF-8"}
//       },
//       cpr::Body{payload.dump()}
//     );
//     auto status_code = r.status_code;
//     auto response_text = r.text;

//     EXPECT_TRUE(r.status_code < 400);
// }

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
