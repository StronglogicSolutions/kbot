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

enum QuestionType {
  UNKNOWN = 0,
  WHAT = 1,
  WHERE = 2,
  WHY = 3,
  WHO = 4,
  WHEN = 5,
  HOW = 6,
  CAN = 7,
  COULD = 8,
  IS = 9,
  TRANSLATE = 10
};

// namespace constants {
const uint8_t QTYPE_Unknown_INDEX = 0;
const uint8_t QTYPE_unknown_INDEX = 1;
const uint8_t QTYPE_What_INDEX = 2;
const uint8_t QTYPE_what_INDEX = 3;
const uint8_t QTYPE_Where_INDEX = 4;
const uint8_t QTYPE_where_INDEX = 5;
const uint8_t QTYPE_Why_INDEX = 6;
const uint8_t QTYPE_why_INDEX = 7;
const uint8_t QTYPE_Who_INDEX = 8;
const uint8_t QTYPE_who_INDEX = 9;
const uint8_t QTYPE_When_INDEX = 10;
const uint8_t QTYPE_when_INDEX = 11;
const uint8_t QTYPE_How_INDEX = 12;
const uint8_t QTYPE_how_INDEX = 13;
const uint8_t QTYPE_Can_INDEX = 14;
const uint8_t QTYPE_can_INDEX = 15;
const uint8_t QTYPE_Could_INDEX = 16;
const uint8_t QTYPE_could_INDEX = 17;
const uint8_t QTYPE_Is_INDEX = 18;
const uint8_t QTYPE_is_INDEX = 19;
const uint8_t QTYPE_Translate_INDEX = 20;
const uint8_t QTYPE_translate_INDEX = 21;

const std::vector<std::string> QTypeNames{
  "Unknown",
  "unknown",
  "What",
  "what",
  "Where",
  "where",
  "Why",
  "why",
  "Who",
  "who",
  "When",
  "when",
  "How",
  "how",
  "Can",
  "can",
  "Could",
  "could",
  "Is",
  "is",
  "Translate",
  "translate"
};
// } // namespace constants

struct ObjectiveContext {
bool         is_continuing;
bool         is_question;
QuestionType question_type;

std::string toString() {
  if (is_question) {
    auto q_index = (question_type == QTYPE_Unknown_INDEX) ? 1 : (question_type * 2);
    return "Is " + QTypeNames.at(q_index) + " question";
  }
  if (is_continuing) {
    return "Is a continuation";
  }

  return "Unknown";
}
};

struct SubjectiveContext {
SubjectiveContext(std::string subject)
: idx{1} {
  subjects[0] = subject;
  subjects[1] = "";
  subjects[2] = "";
}

const std::string& operator[] (uint8_t i) const {
  if (i < 3)
    return subjects[i];
  return subjects[0];
}

const std::string Current() const {
  if (idx == 0)
    return subjects[2];
  return subjects[idx - 1];
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
  const std::string         text;
  const bool                received;
        Message*            next;
        SubjectiveContext*  subjective;
        ObjectiveContext*   objective;
};

using Map               = std::map<const std::string, Message*>;
using MessageObjects    = std::deque<Message>;
using SubjectContexts   = std::deque<SubjectiveContext>;
using ObjectiveContexts = std::deque<ObjectiveContext>;
using Tokens            = std::vector<Token>;

} // namespace conversation

#endif // __NLP_TYPES_HPP__
