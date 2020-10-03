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

  /**
   * loop
   *
   * The loop method runs on its own thread
   */
  virtual void loop() override {
    YouTubeDataAPI* api = static_cast<YouTubeDataAPI*>(m_api.get());

    while (m_is_running) {
      LiveMessages messages = api->FindMentions();

      if (messages.empty()) {
        messages = api->GetChats().at(api->GetLiveDetails().chat_id);
      }

      for (const auto& message : messages) {
        std::cout << message.timestamp << " - " << message.author << ": " << message.text << std::endl;
        std::string tokenized = TokenizeText(message.text);
        std::cout << "Tokenized: \n" << tokenized << std::endl;
      }

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
