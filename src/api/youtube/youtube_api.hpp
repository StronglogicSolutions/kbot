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
namespace kbot {
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
  std::string         GetUsername() { return m_username; }
  std::string         GetBearerAuth();
  AuthData            GetAuth();
  VideoDetails        GetLiveDetails();
  LiveChatMap         GetChats();
  LiveMessages        GetCurrentChat(bool keep_messages = false);
  LiveMessages        FindMentions(bool keep_messages = false);

  bool                FindChat();
  bool                HasChats();
  bool                ClearChat(std::string id = "");
  bool                PostMessage(std::string message);
  bool                ParseTokens();
  bool                InsertMessages(std::string id, LiveMessages&& messages);
  bool                GreetOnEntry();
  bool                HasInteracted(std::string id, Interaction interaction);
  bool                HasDiscussed(std::string value, Interaction type);
  void                RecordInteraction(std::string id, Interaction interaction, std::string value);
  void                SetChatMap(LiveChatMap chat_map) { m_chats = chat_map; }
  void                SetVideoDetails(VideoDetails video_details) { m_video_details = video_details; }

protected:
LiveChatMap  m_chats;

private:
  bool                IsNewer(const char* datetime);

  AuthData     m_auth;
  VideoDetails m_video_details;
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

} // namespace kbot

#endif // __YOUTUBE_DATA_API_HPP__