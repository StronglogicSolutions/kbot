#ifndef __KOREAN_HPP__
#define __KOREAN_HPP__

#include "api/api.hpp"

namespace constants {

const std::vector<std::string> KoreanInterestWords{
"Korea",
"Korean",
"Hangeug",
"Hangug",
"한국",
"한국말",
"사랑해"
};

} // namespace constants

class KoreanAPI : public API{
public:
virtual std::string GetType() override {
  return std::string{"Korean API"};
}

bool MentionsKorean(std::string s) {
  for (const auto& w : constants::KoreanInterestWords) {
    if (s.find(w) != std::string::npos)
      return true;
  }
  return false;
}

};

#endif // __KOREAN_HPP__
