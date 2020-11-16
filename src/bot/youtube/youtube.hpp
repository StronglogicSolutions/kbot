#ifndef __YOUTUBE_HPP__
#define __YOUTUBE_HPP__

#include <iostream>
#include <interfaces/interfaces.hpp>
#include <api/api.hpp>
#include <api/youtube/youtube_api.hpp>
#include <api/korean/korean.hpp>
#include <iterator>
#include <chrono>
#include <ctime>
#include <condition_variable>
#include <mutex>

namespace youtube {

using namespace conversation;

extern const std::string DEFAULT_API_NAME;
extern const std::string DEFAULT_USERNAME;

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
 YouTubeBot();
 virtual void                 loop() override;
 bool                         init();
 std::vector<std::string>     CreateReplyMessages(LiveMessages messages, bool bot_was_mentioned = false);
 virtual std::unique_ptr<API> GetAPI(std::string name = "") override;
 LiveChatMap                  GetChats();
 std::string                  GetResults();
 bool                         PostMessage(std::string message);

private:
  std::unique_ptr<API>     m_api;
  bool                     m_is_own_livestream;
  bool                     m_has_promoted;
  clock_t                  m_time_value;
  std::vector<std::string> m_posted_messages;
  NLP                      m_nlp;
};

} // namespace youtube

#endif // __YOUTUBE_HPP__
