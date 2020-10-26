#ifndef __NLP_TYPES_HPP__
#define __NLP_TYPES_HPP__

#include <deque>
#include <map>
#include <string>

namespace conversation {
enum TokenType {
  location       = 0x00,
  person         = 0x01,
  organization   = 0x02,
  unknown        = 0xFF
};

struct Token {
  TokenType   type;
  std::string value;
};

struct Context {
std::string user;
std::string subject;
Context(std::string user_name, std::string subject_name) : user(user_name), subject(subject_name) {}

bool operator <(const Context &rhs) const {
  return (
    user < rhs.user && subject < rhs.subject
  );
}
};

struct Message {
  std::string       text;
  bool              received;
  const Message*    next;
};

using Map            = std::map<Context, const Message*>;
using MessageObjects = std::deque<Message>;

} // namespace conversation

#endif // __NLP_TYPES_HPP__
