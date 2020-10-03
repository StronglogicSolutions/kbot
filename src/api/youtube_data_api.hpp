#ifndef __YOUTUBE_DATA_API_HPP__
#define __YOUTUBE_DATA_API_HPP__

#include "api/api.hpp"
#include "util/process.hpp"

#include <iostream>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <type_traits>
#include <INIReader.h>

using json = nlohmann::json;

template<typename T>
void log(T s) {
  std::cout << s << std::endl;
}

namespace constants {
// URL Indexes
const uint8_t SEARCH_URL_INDEX         = 0x00;
const uint8_t VIDEOS_URL_INDEX         = 0x01;
const uint8_t LIVE_CHAT_URL_INDEX      = 0x02;
// Header Name Indexes
const uint8_t ACCEPT_HEADER_INDEX      = 0x00;
const uint8_t AUTH_HEADER_INDEX        = 0x01;
const uint8_t CONTENT_TYPE_INDEX       = 0x02;
// Header Value Indexes
const uint8_t APP_JSON_INDEX           = 0x00;
// Param Name Indexes
const uint8_t PART_INDEX               = 0x00;
const uint8_t CHAN_ID_INDEX            = 0x01;
const uint8_t EVENT_T_INDEX            = 0x02;
const uint8_t TYPE_INDEX               = 0x03;
const uint8_t KEY_INDEX                = 0x04;
const uint8_t ID_INDEX                 = 0x05;
const uint8_t LIVE_CHAT_ID_INDEX       = 0x06;
// Param Value Indexes
const uint8_t SL_CHAN_KEY_INDEX        = 0x00;
const uint8_t LIVE_EVENT_TYPE_INDEX    = 0x01;
const uint8_t SNIPPET_INDEX            = 0x02;
const uint8_t VIDEO_TYPE_INDEX         = 0x03;
const uint8_t LIVESTREAM_DETAILS_INDEX = 0x04;
const uint8_t KY_CHAN_KEY_INDEX        = 0x05;

// Strings
const std::vector<std::string> URL_VALUES{
  "https://www.googleapis.com/youtube/v3/search",
  "https://www.googleapis.com/youtube/v3/videos",
  "https://www.googleapis.com/youtube/v3/liveChat/messages"
};

const std::vector<std::string> HEADER_NAMES{
  "Accept",
  "Authorization",
  "Content-Type"
};

const std::vector<std::string> HEADER_VALUES{
  "application/json"
};

const std::vector<std::string> PARAM_NAMES{
  "part",
  "channelId",
  "eventType",
  "type",
  "key",
  "id",
  "liveChatId"
};

const std::vector<std::string> PARAM_VALUES{
  // "UCK0xH_L9OBM0CVwC438bMGA",  // StrongLogic Solutions
  // "UCm5J1Fu_dHgBcMTpXu-NXUw",  // Pangburn
  "UCLwNTXWEjVd2qIHLcXxQWxA",     // Timecast IRL

  "live",
  "snippet",
  "video",
  "liveStreamingDetails",
  "S15j0LRydks",
};

const std::string E_CHANNEL_ID{"UCFP7BAwQIzqml"};
const std::string DEFAULT_CONFIG_PATH{"config/config.ini"};
const std::string YOUTUBE_KEY{"key"};
const std::string YOUTUBE_CONFIG_SECTION{"youtube"};
const std::string YOUTUBE_TOKEN_APP{"token_app"};
const std::string YOUTUBE_USERNAME{"chat_name"};
} // namespace constants

struct AuthData {
  std::string access_token;
  std::string scope;
  std::string token_type;
  std::string expiry_date;
  std::string key;
  std::string token_app_path;
};

struct VideoDetails {
  std::string id;
  std::string chat_id;
};

struct LiveMessage {
  std::string timestamp;
  std::string author;
  std::string text;
};

using LiveMessages = std::vector<LiveMessage>;
using Chat         = std::pair<std::string, LiveMessages>;
using ChatMessages = std::map<std::string, LiveMessages>;

void SanitizeJSON(std::string& s) {
  s.erase(
    std::remove(s.begin(), s.end(),'\"'),
    s.end()
  );
}


class YouTubeDataAPI : public API {
public:
  YouTubeDataAPI () {
    INIReader reader{constants::DEFAULT_CONFIG_PATH};

    if (reader.ParseError() < 0) {
      log("Error loading config");
    }

    auto youtube_key = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_KEY, "");
    if (!youtube_key.empty()) {
      m_auth.key = youtube_key;
    }

    auto app_path = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_TOKEN_APP, "");
    if (!app_path.empty()) {
      m_auth.token_app_path = app_path;
    }

    auto username = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_USERNAME, "");
    if (!username.empty()) {
      m_username = username;
    }

    if (m_auth.token_app_path.empty() || m_auth.key.empty()) {
      throw std::invalid_argument{"Cannot run YouTube API without key and token app"};
    }
  }

  virtual std::string GetType() override {
    return std::string{"YouTube Data API"};
  }

  std::string GetBearerAuth() {
    if (m_auth.access_token.empty()) return "";
    return std::string{"Bearer " + m_auth.access_token};
  }

  /**
   * FetchToken
   */
  std::string FetchToken() {
    if (m_auth.access_token.empty()) {

      ProcessResult result = qx({m_auth.token_app_path});

      if (result.error) {
        std::cout << "Error executing program to retrieve token" << std::endl;
        return "";
      }

      json auth_json = json::parse(result.output);

      if (auth_json.empty()) {
        return "";
      }

      if (!auth_json.is_null() && auth_json.is_object()) {
        m_auth.access_token = auth_json["access_token"].dump();
        m_auth.scope        = auth_json["scope"].dump();
        m_auth.token_type   = auth_json["token_type"].dump();
        m_auth.expiry_date  = auth_json["expiry_date"].dump();
      }
    }

    return m_auth.access_token;
  }

  AuthData GetAuth() {
    return m_auth;
  }

  /**
   * FetchLiveVideoID
   */
  std::string FetchLiveVideoID() {
    using namespace constants;
    if (m_auth.access_token.empty())
      throw std::invalid_argument("Cannot use YouTube API without access token");

    cpr::Response r = cpr::Get(
      cpr::Url{URL_VALUES.at(SEARCH_URL_INDEX)},
      cpr::Header{
        {HEADER_NAMES.at(ACCEPT_HEADER_INDEX), HEADER_VALUES.at(APP_JSON_INDEX)},
        {HEADER_NAMES.at(AUTH_HEADER_INDEX),   GetBearerAuth()}
      },
      cpr::Parameters{
        {PARAM_NAMES.at(PART_INDEX),    PARAM_VALUES.at(SNIPPET_INDEX)},
        {PARAM_NAMES.at(KEY_INDEX),     m_auth.key},
        {PARAM_NAMES.at(CHAN_ID_INDEX), PARAM_VALUES.at(SL_CHAN_KEY_INDEX)},
        {PARAM_NAMES.at(EVENT_T_INDEX), PARAM_VALUES.at(LIVE_EVENT_TYPE_INDEX)},
        {PARAM_NAMES.at(TYPE_INDEX),    PARAM_VALUES.at(VIDEO_TYPE_INDEX)}
      }
    );

    std::cout << r.status_code << std::endl;
    std::cout << r.header["content-type"] << std::endl;;
    std::cout << r.text << std::endl;

    json video_info = json::parse(r.text);

    if (!video_info.is_null() && video_info.is_object()) {
      auto items = video_info["items"];
      if (!items.is_null() && items.is_array() && items.size() > 0) {
        m_video_details.id = items[0]["id"]["videoId"].dump();
        SanitizeJSON(m_video_details.id);
      }
    }

    return m_video_details.id;
  }

  /**
   * GetLiveDetails
   */
  VideoDetails GetLiveDetails() {
    return m_video_details;
  }

  /**
   * FetchLiveDetails
   */
  bool FetchLiveDetails() {
    using namespace constants;
    if (m_auth.access_token.empty())
      throw std::invalid_argument{"Unable to use YouTubeDataAPI without token"};

    if (m_video_details.id.empty())
      throw std::invalid_argument{"Unable to Fetch live details: no video ID"};

    cpr::Response r = cpr::Get(
      cpr::Url{URL_VALUES.at(VIDEOS_URL_INDEX)},
      cpr::Header{
        {HEADER_NAMES.at(ACCEPT_HEADER_INDEX), HEADER_VALUES.at(APP_JSON_INDEX)},
        {HEADER_NAMES.at(AUTH_HEADER_INDEX),   GetBearerAuth()}
      },
      // ,
      cpr::Parameters{
        {PARAM_NAMES.at(PART_INDEX),    PARAM_VALUES.at(LIVESTREAM_DETAILS_INDEX)},
        {PARAM_NAMES.at(KEY_INDEX),     m_auth.key},
        {PARAM_NAMES.at(ID_INDEX),      m_video_details.id}
      }
    );

    std::cout << r.status_code << std::endl;
    std::cout << r.header["content-type"] << std::endl;;
    std::cout << r.text << std::endl;

    json live_info = json::parse(r.text);

    if (!live_info.is_null() && live_info.is_object()) {
      auto items = live_info["items"];
      if (!items.is_null() && items.is_array() && items.size() > 0) {
        m_video_details.chat_id = items[0]["liveStreamingDetails"]["activeLiveChatId"].dump();
        SanitizeJSON(m_video_details.chat_id);
        if (!m_video_details.chat_id.empty()) {
          m_chats.insert({m_video_details.chat_id, std::vector<LiveMessage>{}});
          return true;
        }
      }
    }
    return false;
  }

  /**
   * GetChatMessages
   */
  std::string FetchChatMessages() {
  using namespace constants;
    if (m_auth.access_token.empty() || m_video_details.chat_id.empty()) return "";

    cpr::Response r = cpr::Get(
      cpr::Url{URL_VALUES.at(LIVE_CHAT_URL_INDEX)},
      cpr::Header{
        {HEADER_NAMES.at(ACCEPT_HEADER_INDEX), HEADER_VALUES.at(APP_JSON_INDEX)},
        {HEADER_NAMES.at(AUTH_HEADER_INDEX),   GetBearerAuth()}
      },
      cpr::Parameters{
        {PARAM_NAMES.at(PART_INDEX),           PARAM_VALUES.at(SNIPPET_INDEX)},
        {PARAM_NAMES.at(KEY_INDEX),            m_auth.key},
        {PARAM_NAMES.at(LIVE_CHAT_ID_INDEX),   m_video_details.chat_id},
      }
    );

    std::cout << r.status_code << std::endl;
    std::cout << r.header["content-type"] << std::endl;;
    std::cout << r.text << std::endl;

    json chat_info = json::parse(r.text);

    if (!chat_info.is_null() && chat_info.is_object()) {
      auto items = chat_info["items"];

      if (!items.is_null() && items.is_array()) {
        for (const auto& item : items) {
          std::string text   = item["snippet"]["textMessageDetails"]["messageText"];
          std::string author = item["snippet"]["authorChannelId"];
          std::string time   = item["snippet"]["publishedAt"];
          SanitizeJSON(text);
          SanitizeJSON(author);
          SanitizeJSON(time);

          m_chats.at(m_video_details.chat_id).push_back(
            LiveMessage{
              .timestamp = time,
              .author    = author,
              .text      = text
            }
          );
        }
      }
    }

    return r.text;
  }

  ChatMessages GetChats() {
    return m_chats;
  }

  LiveMessages FindMentions() {
    using ChatPair = std::map<std::string, LiveMessages>;
    const std::string bot_name{""};

    LiveMessages matches{};

    for (const Chat& m : GetChats()) {
      auto chat_name        = m.first;
      LiveMessages messages = m.second;

      for (const LiveMessage& message : messages) {
        if (message.text.find(bot_name) != std::string::npos) {
          matches.push_back(message);
        }
      }
    }
    return matches;
  }

  bool FindChat() {
    if (m_auth.access_token.empty()) {
      if (GetAuth().access_token.empty()) {
        return false;
      }
    }

    if (FetchLiveVideoID().empty()) {
      return false;
    }

    if (!FetchLiveDetails()) {
      return false;
    }

    FetchChatMessages();

    return true;
  }

  bool PostMessage(std::string message) {
    using namespace constants;

    if (m_video_details.chat_id.empty()) {
      log("No chat to post to");
      return false;
    }

    json payload{};
    payload["snippet"]["liveChatId"]                        = m_video_details.chat_id;
    payload["snippet"]["live_chat_id"]                      = m_video_details.chat_id;
    payload["snippet"]["textMessageDetails"]["messageText"] = message;
    payload["snippet"]["type"]                              = "textMessageEvent";

    cpr::Response r = cpr::Post(
      cpr::Url{URL_VALUES.at(LIVE_CHAT_URL_INDEX)},
      cpr::Header{
        {HEADER_NAMES.at(ACCEPT_HEADER_INDEX), HEADER_VALUES.at(APP_JSON_INDEX)},
        {HEADER_NAMES.at(AUTH_HEADER_INDEX),   GetBearerAuth()},
        {HEADER_NAMES.at(CONTENT_TYPE_INDEX),  HEADER_VALUES.at(APP_JSON_INDEX)}
      },
      cpr::Parameters{
        {PARAM_NAMES.at(PART_INDEX),           PARAM_VALUES.at(SNIPPET_INDEX)},
        {PARAM_NAMES.at(KEY_INDEX),            m_auth.key},
        {PARAM_NAMES.at(LIVE_CHAT_ID_INDEX),   m_video_details.chat_id},
      },
      cpr::Body{payload.dump()}
    );

    std::cout << r.status_code << std::endl;
    std::cout << r.header["content-type"] << std::endl;;
    std::cout << r.text << std::endl;

    return true;
  }

private:
  AuthData     m_auth;
  VideoDetails m_video_details;
  ChatMessages m_chats;
  std::string  m_active_chat;
  std::string  m_username;
};

#endif // __YOUTUBE_DATA_API_HPP__