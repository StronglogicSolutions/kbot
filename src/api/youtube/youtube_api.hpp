#ifndef __YOUTUBE_DATA_API_HPP__
#define __YOUTUBE_DATA_API_HPP__

#include <iostream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <INIReader.h>

#include "api/api.hpp"
#include "api/nlp/nlp.hpp"
#include "bot/youtube/types.hpp"
#include "util/util.hpp"
#include "util/process.hpp"

using json = nlohmann::json;


namespace youtube {
const std::string CreateLocationResponse(std::string location);
const std::string CreatePersonResponse(std::string name);
const std::string CreateOrganizationResponse(std::string name);
const std::string CreatePromoteResponse(bool test_mode = false);

class YouTubeDataAPI : public API {
public:
  YouTubeDataAPI();

  virtual std::string GetType()  override;
  virtual bool        TestMode() override;

  std::string         FetchToken();
  std::string         FetchLiveVideoID();
  bool                FetchLiveDetails();
  std::string         FetchChatMessages();

  std::string         GetBearerAuth();
  AuthData            GetAuth();
  VideoDetails        GetLiveDetails();
  LiveChatMap         GetChats();
  LiveMessages        GetCurrentChat(bool keep_messages = false);

  bool                InsertMessages(std::string id, LiveMessages&& messages);
  bool                ClearChat(std::string id = "");
  LiveMessages        FindMentions(bool keep_messages = false);
  bool                FindChat();
  bool                PostMessage(std::string message);
  bool                HasChats();
  bool                GreetOnEntry();
  bool                HasInteracted(std::string id, Interaction interaction);
  bool                HasDiscussed(std::string value, Interaction type);
  bool                IsNewer(const char* datetime);
  void                RecordInteraction(std::string id, Interaction interaction, std::string value);

  static TokenType    GetType(std::string type);
  static Token        ParseToken(std::string s);
  static std::vector<Token> SplitTokens(std::string s);
  bool                ParseTokens();

private:
  AuthData     m_auth;
  VideoDetails m_video_details;
  LiveChatMap  m_chats;
  ActivityMap  m_interactions;
  LocationMap  m_locations;
  PersonMap    m_persons;
  OrgMap       m_orgs;
  std::string  m_active_chat;
  std::string  m_username;
  std::time_t  m_last_fetch_timestamp;
  bool         m_greet_on_entry;
  bool         m_test_mode;
  bool         m_retry_mode;
};

} // namespace youtube

#endif // __YOUTUBE_DATA_API_HPP__