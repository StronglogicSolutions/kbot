#pragma once

#include <vector>
#include <string>
#include <string_view>

namespace constants {
// URL Indexes
extern const uint8_t SEARCH_URL_INDEX;
extern const uint8_t VIDEOS_URL_INDEX;
extern const uint8_t LIVE_CHAT_URL_INDEX;

// Header Name Indexes
extern const uint8_t ACCEPT_HEADER_INDEX;
extern const uint8_t AUTH_HEADER_INDEX;
extern const uint8_t CONTENT_TYPE_INDEX;

// Header Value Indexes
extern const uint8_t APP_JSON_INDEX;

// Param Name Indexes
extern const uint8_t PART_INDEX;
extern const uint8_t CHAN_ID_INDEX;
extern const uint8_t EVENT_T_INDEX;
extern const uint8_t TYPE_INDEX;
extern const uint8_t KEY_INDEX;
extern const uint8_t ID_INDEX;
extern const uint8_t LIVE_CHAT_ID_INDEX;

// Param Value Indexes
extern const uint8_t SL_CHAN_KEY_INDEX;
extern const uint8_t LIVE_EVENT_TYPE_INDEX;
extern const uint8_t SNIPPET_INDEX;
extern const uint8_t VIDEO_TYPE_INDEX;
extern const uint8_t LIVESTREAM_DETAILS_INDEX;
extern const uint8_t KY_CHAN_KEY_INDEX;

// Strings
extern const std::vector<std::string> URL_VALUES;

extern const std::vector<std::string> HEADER_NAMES;

extern const std::vector<std::string> HEADER_VALUES;

extern const std::vector<std::string> PARAM_NAMES;

extern const std::vector<std::string> PARAM_VALUES;

extern const std::string E_CHANNEL_ID;
extern const std::string DEFAULT_CONFIG_PATH;
extern const std::string YOUTUBE_KEY;
extern const std::string YOUTUBE_CONFIG_SECTION;
extern const std::string YOUTUBE_TOKEN_APP;
extern const std::string YOUTUBE_USERNAME;
extern const std::string YOUTUBE_GREET;
extern const std::string YOUTUBE_TEST_MODE;
extern const std::string YOUTUBE_RETRY_MODE;

namespace invitations {
extern const std::string OFFER_TO_INQUIRE;
} // namespace invitations

namespace promotion {
extern const std::string support;
extern const std::string test_support;
} // namespace promotion

// const std::vector<std::string
} // namespace constants
