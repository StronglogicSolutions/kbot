#pragma once

#include <vector>
#include <map>
#include <string>
#include <nlohmann/json.hpp>
#include <type_traits>
#include "api/nlp/types.hpp"
#include "api/youtube/constants.hpp"

namespace kbot {
using namespace conversation;
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
  std::string title;
  std::string description;
  std::string channel_title;
  std::string channel_id;
  std::string url;
  std::string thumbnail;
};

struct LiveMessage {
  std::string        timestamp;
  std::string        author;
  std::string        text;
  std::vector<Token> tokens;
};

struct UserInteraction {
  std::string id;
  bool        greeted;
  bool        promoted;
  bool        probed;
  bool        location;
};

enum Interaction {
  greeting      = 0x00,
  promotion     = 0x01,
  probing       = 0x02,
  location_ask  = 0x03
};

using LiveMessages = std::vector<LiveMessage>;
using Chat         = std::pair<std::string, LiveMessages>;
using LiveChatMap  = std::map<std::string, LiveMessages>;
using ActivityMap  = std::map<std::string, UserInteraction>;
using LocationMap  = std::map<std::string, bool>;
using PersonMap    = LocationMap;
using OrgMap       = LocationMap;
}
