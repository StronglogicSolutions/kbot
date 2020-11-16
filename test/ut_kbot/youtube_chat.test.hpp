#ifndef __YOUTUBE_CHAT_TEST_HPP__
#define __YOUTUBE_CHAT_TEST_HPP__

#include <string>
#include <iostream>

#include "gtest/gtest.h"

#include "api/youtube/youtube_api.hpp"

const std::string           TEST_USERNAME{"Emmanuel Buckshi"};
const std::string           TEST_CHAT_ID{"ooooooooooooorf"};
const std::string           TEST_AUTHOR_1{"Jazilla"};
const std::string           TEST_AUTHOR_2{"Jizzimiah"};
const std::string           TEST_AUTHOR_3{"Jizzachanezzard"};

const youtube::VideoDetails TEST_VIDEO_DETAILS{
  .id      = "Random ID",
  .chat_id = TEST_CHAT_ID
};

class MockYouTubeDataAPI : public youtube::YouTubeDataAPI {
public:

void InsertMessages(std::string key, youtube::LiveMessages messages) {
  for (const auto& message : messages) {
    m_chats.at(key).push_back(message);
  }
}
};


class YouTubeChatTestFixture: public ::testing::Test {
 protected:
  MockYouTubeDataAPI      api;
  conversation::NLP       nlp;

  void SetUp() override {

  }

  void TearDown() override {

  }

  std::vector<youtube::LiveMessage> GetLiveMessageSet1() { // 7
    return std::vector<youtube::LiveMessage>{
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:00:00",
        .author = TEST_AUTHOR_1,
        .text   = "Fantastic. Nice to meet you, @Emmanuel Buckshi!",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:00:01",
        .author = TEST_AUTHOR_1,
        .text   = "@Emmanuel Buckshi: why, yes I have heard of StrongLogic solutions",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:00:02",
        .author = TEST_AUTHOR_2,
        .text   = "My name is Gilbert and noble count of Huedin",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:00:03",
        .author = TEST_AUTHOR_2,
        .text   = "@Emmanuel Buckshi! Huedin is in Romania",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:00:04",
        .author = TEST_AUTHOR_3,
        .text   = "I am a big fan of the World Health Organization (WHO)",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:00:04",
        .author = TEST_AUTHOR_1,
        .text   = "Have you ever been to China?",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:00:04",
        .author = TEST_AUTHOR_1,
        .text   = "I wish I could live in India",
        .tokens = std::vector<conversation::Token>{}
      }    };
  }

  std::vector<youtube::LiveMessage> GetLiveMessageSet2() { // 5
    return std::vector<youtube::LiveMessage>{
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:01:00",
        .author = TEST_AUTHOR_1,
        .text   = "@Emmanuel Buckshi: really?",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:01:01",
        .author = TEST_AUTHOR_1,
        .text   = "@Emmanuel Buckshi: does Stronglogic have representation in India?",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:01:02",
        .author = TEST_AUTHOR_2,
        .text   = "@Emmanuel Buckshi: people only ever know about Transylvania",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:01:03",
        .author = TEST_AUTHOR_2,
        .text   = "@Emmanuel Buckshi: have you ever been to Europe?",
        .tokens = std::vector<conversation::Token>{}
      },
      youtube::LiveMessage{
        .timestamp = "2020-10-26T12:01:04",
        .author = TEST_AUTHOR_3,
        .text   = "@Emmanuel Buckshi: What's wrong with the World Health Organization?",
        .tokens = std::vector<conversation::Token>{}
      }
    };
  }


 public:
  YouTubeChatTestFixture()
  : api(MockYouTubeDataAPI{}),
    nlp(conversation::NLP{"@Emmanuel Buckshi"}) {}
};

#endif // __YOUTUBE_CHAT_TEST_HPP__
