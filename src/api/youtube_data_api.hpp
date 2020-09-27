#ifndef __YOUTUBE_DATA_API_HPP__
#define __YOUTUBE_DATA_API_HPP__

#include "api/api.hpp"
#include "util/process.hpp"

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <type_traits>

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
  "Authorization"
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
  "UCK0xH_L9OBM0CVwC438bMGA",
  "live",
  "snippet",
  "video",
  "liveStreamingDetails",
  "S15j0LRydks",
};
} // namespace constants

struct AuthData {
  std::string access_token;
  std::string scope;
  std::string token_type;
  std::string expiry_date;
  std::string key;
};

struct VideoDetails {
  std::string id;
  std::string chat_id;
};

void SanitizeJSON(std::string& s) {
  s.erase(
    std::remove(s.begin(), s.end(),'\"'),
    s.end()
  ); // Remove double quotes
}


class YouTubeDataAPI : public API {
public:
  virtual std::string GetType() override {
    return std::string{"YouTube Data API"};
  }

  std::string GetBearerAuth() {
    if (m_auth.access_token.empty()) return "";
    return std::string{"Bearer " + m_auth.access_token};
  }

  /**
   * GetToken
   */
  std::string GetToken() {
    ProcessResult result = qx({"/data/www/kiggle/get_token.js"});
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
      // TODO: Create solution to read key
      m_auth.key = "";
    }

    return result.output;
  }

  AuthData GetAuth() {
    return m_auth;
  }

  /**
   * GetLiveVideoID
   */
  std::string GetLiveVideoID() {
    using namespace constants;
    if (m_auth.access_token.empty()) return "";

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

    return r.text;
  }

  /**
   * GetLiveDetails
   */
  std::string GetLiveDetails() {
    using namespace constants;
    if (m_auth.access_token.empty() || m_video_details.id.empty()) return "";
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
      }
    }

    return r.text;
  }

  /**
   * GetChatMessages
   */
  std::string GetChatMessages() {
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

    return r.text;
  }

private:
  AuthData     m_auth;
  VideoDetails m_video_details;
};

#endif // __YOUTUBE_DATA_API_HPP__