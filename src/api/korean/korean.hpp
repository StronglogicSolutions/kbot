#ifndef __KOREAN_HPP__
#define __KOREAN_HPP__

#include "api/api.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace korean {
namespace constants {
extern const std::vector<std::string> KoreanInterestWords;

} // namespace constants

namespace request {
void MakeRequest(std::string text);
} // namespace request

namespace translation {
std::string ExtractSubtext(std::string text);

std::string TranslateText(std::string text);
} // namespace translation

std::string TranslateToKorean(std::string text);


class KoreanAPI : public API{
public:

KoreanAPI() {
  // TODO: load config
}

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

std::string Translate(std::string s) {
  return translation::TranslateText(s);
}

};
} // namespace korean

#endif // __KOREAN_HPP__
