#pragma once

#include <iostream>
#include <iterator>
#include <chrono>
#include <ctime>
#include <condition_variable>
#include <mutex>

#include <interfaces/interfaces.hpp>
#include <api/api.hpp>
// #include <api/korean/korean.hpp>

#include <ktube/ktube.hpp>
#include <psqlorm.hpp>

namespace kbot {

// using namespace conversation;
// using namespace ktube;
// using namespace korean;
using LiveMessages   = ktube::LiveMessages;
using LiveChatMap    = ktube::LiveChatMap;
using YouTubeDataAPI = ktube::YouTubeDataAPI;

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
  virtual bool                 IsRunning() override;
  virtual void                 Init() override;
  virtual void                 Start() override;
  virtual void                 Shutdown() override;
  virtual void                 SetCallback(BrokerCallback cb_fn) override;
  virtual bool                 HandleEvent(BotRequest event) override;
  LiveChatMap                  GetChats();
  std::string                  GetResults();
  bool                         PostMessage(std::string message);
  void                         UpdateChats();
  bool                         HaveCommented(const std::string& vid);
  bool                         HaveReplied(const std::string& cid);
  bool                         IsOwnComment(const std::string& cid);
  bool                         InsertComment(const ktube::Comment& comment);

private:
  YouTubeDataAPI           m_api;
  // korean::KoreanAPI        m_korean_api;
  bool                     m_is_own_livestream;
  bool                     m_has_promoted;
  clock_t                  m_time_value;
  std::vector<std::string> m_posted_messages;
  conversation::NLP        m_nlp;
  BrokerCallback           m_send_event_fn;
  Database::PSQLORM        m_db;
};

} // namespace kbot
