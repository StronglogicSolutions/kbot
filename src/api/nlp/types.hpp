#ifndef __NLP_TYPES_HPP__
#define __NLP_TYPES_HPP__

#include <deque>
#include <map>
#include <string>
#include <stdio.h>

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

struct CompositeContext {
std::string user;
std::string subject;
CompositeContext(std::string user_name, std::string subject_name) : user(user_name), subject(subject_name) {}

bool operator <(const CompositeContext &rhs) const {
  auto user_comp = user.compare(rhs.user);
  if (user_comp < 0) {
    return true;
  }
  else
  if (user_comp > 0) {
    return false;
  }
  else {
    auto subject_comp = subject.compare(rhs.subject);
    if (subject_comp < 0) {
      return true;
    }
    else
      return true;
  }
}
};

struct Context {
Context(std::string subject)
: idx{1} {
  subjects[0] = subject;
  subjects[1] = "";
  subjects[2] = "";
}

const std::string& operator[] (uint8_t i) const {
  if (i < 3)
    return subjects[i];
  return "";
}

void Insert(std::string s) {
  subjects[idx] = s;

  (idx == 2) ? idx = 0 : idx++;
}

std::string toString() {
  std::string s{};
  s.reserve(subjects[0].size() + subjects[1].size() + subjects[2].size());

  s += subjects[0];

  if (!subjects[1].empty())
    s +=  ", " + subjects[1];
  if (!subjects[2].empty())
    s +=  ", " + subjects[2];

  return s;
}

std::string subjects[3];
uint8_t     idx;
};

struct Message {
  const std::string text;
  const bool        received;
  const Message*    next;
        Context*    context;
};

using Map            = std::map<const std::string, const Message*>;
using MessageObjects = std::deque<Message>;
using ContextObjects = std::deque<Context>;

} // namespace conversation

#endif // __NLP_TYPES_HPP__
