#ifndef __YOUTUBE_HPP__
#define __YOUTUBE_HPP__

#include <iostream>
#include <interfaces/interfaces.hpp>
#include <api/api.hpp>
#include <api/youtube/api.hpp>
#include <api/korean/korean.hpp>
#include <chrono>
#include <ctime>
#include <condition_variable>
#include <mutex>

const std::string DEFAULT_API_NAME{"YouTube Data API"};

/**
 * YouTubeBot
 *
 * @interface {Bot}
 * @interface {Worker}
 *
 * Specializes in using the Google YouTube Data API
 *
 */
class YouTubeBot : public Bot, public Worker {
 public:
 /**
  * @constructor
  */
  YouTubeBot()
  : Bot("YouTubeBot"),
    m_has_promoted(false),
    m_is_own_livestream(false),
    m_time_value(clock())
  {}

  bool init() {
    m_api               = GetAPI(DEFAULT_API_NAME);
    YouTubeDataAPI* api = static_cast<YouTubeDataAPI*>(m_api.get());

    if (
      !api->FetchToken()      .empty() &&
      !api->FetchLiveVideoID().empty() &&
       api->FetchLiveDetails()
    ) {
      api->FetchChatMessages();

      if (api->GreetOnEntry()) {
        api->PostMessage("Hello");
      }

      return true;
    }
    return false;
  }

  std::vector<std::string> CreateReplyMessages(LiveMessages messages, bool bot_was_mentioned = false) {
    std::vector<std::string> reply_messages{};
    YouTubeDataAPI* api = static_cast<YouTubeDataAPI*>(m_api.get());
    // if (bot_was_mentioned) {
      auto reply_number = (!m_has_promoted) ? messages.size() + 1 : messages.size();
      reply_messages.reserve(reply_number); // Create responses to all who mentioned bot specifically

      for (const auto& message : messages) {
        auto user_id = message.author;
        if (!message.tokens.empty()) {
          for (const auto& token : message.tokens) {
            /**
            ┌───────────────────────────────────────────────────────────┐
            │░░░░░░░░░░░░░░░░░░░░░░TRACKING REPLIES░░░░░░░░░░░░░░░░░░░░░░│
            │░░░░░░░░Track if we have already responded to the author:░░░░░│
            │░░░░░░░░1. For their location                            ░░░░░│
            │░░░░░░░░2. To greet them                                 ░░░░░│
            │░░░░░░░░3. For a particular mention                      ░░░░░│
            └───────────────────────────────────────────────────────────┘
            */

            if (token.type == TokenType::location && !api->HasInteracted(user_id, Interaction::location_ask)) {
              reply_messages.push_back(CreateLocationResponse(token.value));
              api->RecordInteraction(message.author, Interaction::location_ask, token.value);
            }
            else
            if (token.type == TokenType::person && !api->HasInteracted(user_id, Interaction::greeting)) {
              reply_messages.push_back(CreatePersonResponse(token.value));
              api->RecordInteraction(message.author, Interaction::greeting, token.value);
            }
            else
            if (token.type == TokenType::organization && !api->HasInteracted(user_id, Interaction::probing)) {
              reply_messages.push_back(CreateOrganizationResponse(token.value));
              api->RecordInteraction(message.author, Interaction::probing, token.value);
            }
          }
        }
      }
    // } else {
    //   reply_messages.reserve((!m_has_promoted) ? 2 : 1);
    //   reply_messages.push_back("Hello from the KBot!");
    // }

    // if (!m_has_promoted) {
    //   reply_messages.push_back(CreatePromoteResponse());
    //   m_has_promoted = true;
    // }

    return reply_messages;
  }

  /**
   * loop
   *
   * The loop method runs on its own thread
   */
  virtual void loop() override {
    YouTubeDataAPI* api = static_cast<YouTubeDataAPI*>(m_api.get());

    uint8_t no_hits{0};

    while (m_is_running) {
      api->ParseTokens();

      if (api->HasChats()) {
        bool bot_was_mentioned = false;
        LiveMessages messages = api->FindMentions();

        if (!messages.empty()) {
          bot_was_mentioned = true;
          log("Bot was mentioned");
        }
        // else { // We are doing this for now
        messages = api->GetCurrentChat();
        api->ClearChat();
        // }
        auto k_api = GetAPI("Korean API");
        KoreanAPI* korean_api = static_cast<KoreanAPI*>(k_api.get());
        for (const auto& message : messages) {
          if (korean_api->MentionsKorean(message.text)) {
            log("Message from " + message.author + " mentions Korean:\n" + message.text);
          }
        }

        std::vector<std::string> reply_messages = CreateReplyMessages(messages, bot_was_mentioned);
        int max = 5;
        for (const auto& reply : reply_messages) {
          log("Reply message:\n" + reply);
          api->PostMessage(reply);
          if (--max == 0) break;
        }
      } else {
        no_hits++;
      }

      api->FetchChatMessages();

      if (no_hits < 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      } else {
        // Not having much luck. Take a break.
        no_hits = 0;
        std::this_thread::sleep_for(std::chrono::seconds(360));
      }
    }
  }

  /**
   * GetAPI
   *
   * @param
   * @returns
   */
  virtual std::unique_ptr<API> GetAPI(std::string name = "") {
    if (name.empty()) {
      return std::make_unique<DefaultAPI>();
    }
    else
    if (name.compare("Request API") == 0) {
      return std::make_unique<RequestAPI>();
    }
    else
    if (name.compare("YouTube Data API") == 0) {
      return std::make_unique<YouTubeDataAPI>();
    }
    else
    if (name.compare("Korean API") == 0) {
      return std::make_unique<KoreanAPI>();
    }
    return nullptr;
  }

/**
 * GetChats
 *
 * @returns [out] {LiveChatMap}  A map of Live Chats indexed by chat id
 */
LiveChatMap GetChats() {
  if (m_api != nullptr) {
    return static_cast<YouTubeDataAPI*>(m_api.get())->GetChats();
  }
  return LiveChatMap{};
}

/**
 * PostMessage
 *
 * @param   [in]  {std::string}
 * @returns [out] {bool}
 */
bool PostMessage(std::string message) {
  return static_cast<YouTubeDataAPI*>(m_api.get())->PostMessage(message);
}

private:
  std::unique_ptr<API>    m_api;
  bool                    m_is_own_livestream;
  bool                    m_has_promoted;
  std::condition_variable m_work_thread_condition;
  std::mutex              m_mutex;
  clock_t                 m_time_value;
};

#endif // __YOUTUBE_HPP__
