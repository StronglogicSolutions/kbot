#ifndef __YOUTUBE_CONSTANTS_HPP__
#define __YOUTUBE_CONSTANTS_HPP__

#include <vector>
#include <string>

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

const std::vector<std::string> PARAM_NAMES = {
  "part",
  "channelId",
  "eventType",
  "type",
  "key",
  "id",
  "liveChatId"
};

const std::vector<std::string> PARAM_VALUES{
  "UCK0xH_L9OBM0CVwC438bMGA",  // StrongLogic Solutions
  // "UCm5J1Fu_dHgBcMTpXu-NXUw", // Pangburn
  // "UCLwNTXWEjVd2qIHLcXxQWxA", // Timecast IRL
  // "UC1XoiwW6b0VIYPOaP1KgV7A", // KStyleYo

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
const std::string YOUTUBE_GREET{"greet"};

namespace invitations {
std::string Greet(std::string name) {
  return std::string{
    "Hello, " + name + ". How are you?"
  };
}

const std::string OFFER_TO_INQUIRE{
  "Ask me a question: '@' me, and say \"!q <your question>\"\n\
   If I can't respond immediately, someone will get back to you!"
};

namespace promotion {
const std::string support {"Please click like and subscribe!"};
}
} // namespace invitations

// const std::vector<std::string
} // namespace constants

#endif // __YOUTUBE_CONSTANTS_HPP__
