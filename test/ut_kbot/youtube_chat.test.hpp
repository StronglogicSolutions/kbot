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


class YouTubeChatTestFixture: public ::testing::Test {
 protected:
  youtube::YouTubeDataAPI api;
  conversation::NLP       nlp;

  void SetUp() override {

  }

  void TearDown() override {

  }

  std::vector<youtube::LiveMessage> GetLiveMessageSet1() {
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
      }
    };
  }

  // std::vector<youtube::LiveMessage> GetLiveMessageSet2() {
  //   return std::vector<youtube::LiveMessage>{
  //     youtube::LiveMessage{
  //       .timestamp = "2020-10-26T12:00:00",
  //       .author = TEST_AUTHOR_1,
  //       .text   = "Fantastic. Nice to meet you, @Emmanuel Buckshi!",
  //       .tokens = std::vector<conversation::Token>{}
  //     },
  //     youtube::LiveMessage{
  //       .timestamp = "2020-10-26T12:00:00",
  //       .author = TEST_AUTHOR_1,
  //       .text   = "Fantastic. Nice to meet you, @Emmanuel Buckshi!",
  //       .tokens = std::vector<conversation::Token>{}
  //     },
  //     youtube::LiveMessage{
  //       .timestamp = "2020-10-26T12:00:00",
  //       .author = TEST_AUTHOR_1,
  //       .text   = "Fantastic. Nice to meet you, @Emmanuel Buckshi!",
  //       .tokens = std::vector<conversation::Token>{}
  //     },
  //     youtube::LiveMessage{
  //       .timestamp = "2020-10-26T12:00:00",
  //       .author = TEST_AUTHOR_1,
  //       .text   = "Fantastic. Nice to meet you, @Emmanuel Buckshi!",
  //       .tokens = std::vector<conversation::Token>{}
  //     },
  //     youtube::LiveMessage{
  //       .timestamp = "2020-10-26T12:00:00",
  //       .author = TEST_AUTHOR_1,
  //       .text   = "Fantastic. Nice to meet you, @Emmanuel Buckshi!",
  //       .tokens = std::vector<conversation::Token>{}
  //     }
  //   };
  // }


 public:
  YouTubeChatTestFixture()
  : api(youtube::YouTubeDataAPI{}) {}
};

#endif // __YOUTUBE_CHAT_TEST_HPP__
