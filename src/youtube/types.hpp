#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include <vector>
#include <map>
#include <string>
#include <nlohmann/json.hpp>
#include <type_traits>

#include "api/youtube_constants.hpp"

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

enum TokenType {
  location = 0x00,
};

struct Token {
  TokenType   type;
  std::string value;
};

struct LiveMessage {
  std::string        timestamp;
  std::string        author;
  std::string        text;
  std::vector<Token> tokens;
};

using LiveMessages = std::vector<LiveMessage>;
using Chat         = std::pair<std::string, LiveMessages>;
using LiveChatMap  = std::map<std::string, LiveMessages>;

#endif // __TYPES_HPP__
