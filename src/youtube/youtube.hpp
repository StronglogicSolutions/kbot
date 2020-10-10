#ifndef __YOUTUBE_HPP__
#define __YOUTUBE_HPP__

#include <iostream>
#include <interfaces/interfaces.hpp>
#include <api/api.hpp>
#include <api/youtube_data_api.hpp>
#include <chrono>

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
  : Bot("YouTubeBot") {}

  bool init() {
    m_api               = GetAPI(DEFAULT_API_NAME);
    YouTubeDataAPI* api = static_cast<YouTubeDataAPI*>(m_api.get());

    if (
      !api->FetchToken()      .empty() &&
      !api->FetchLiveVideoID().empty() &&
       api->FetchLiveDetails()
    ) {
      api->FetchChatMessages();
      return true;
    }
    return false;
  }

  std::vector<std::string> CreateReplyMessages(LiveMessages messages, bool bot_was_mentioned = false) {
    std::vector<std::string> reply_messages{};
    if (bot_was_mentioned) {
      reply_messages.reserve(messages.size());
    }

    return reply_messages;
  }

  /**
   * loop
   *
   * The loop method runs on its own thread
   */
  virtual void loop() override {
    YouTubeDataAPI* api = static_cast<YouTubeDataAPI*>(m_api.get());

    while (m_is_running) {
      api->ParseTokens();

      if (api->HasChats()) {
        bool bot_was_mentioned = false;
        LiveMessages messages = api->FindMentions();

        if (!messages.empty()) {
          bot_was_mentioned = true;
        } else {
          messages = api->GetCurrentChat();
        }

        std::vector<std::string> reply_messages = CreateReplyMessages(messages, bot_was_mentioned);

        for (const auto& reply : reply_messages) {
          api->PostMessage(reply);
        }
      }

      api->FetchChatMessages();
      // TODO: Switch wait to condition variable
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
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
std::unique_ptr<API> m_api;

};

#endif // __YOUTUBE_HPP__
