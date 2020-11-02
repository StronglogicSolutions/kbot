#include "korean.hpp"

namespace korean {
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

namespace request {
inline void MakeRequest(std::string text) {
  json payload{};
  payload["text"] = text;
  payload["source"] = "en";
  payload["target"] = "ko";
  cpr::Response r = cpr::Post(
    cpr::Url{"https://openapi.naver.com/v1/papago/n2mt"},
    cpr::Header{
     {"X-Naver-Client-Id", ""},
     {"X-Naver-Client-Secret",   ""},
     {"Content-Type", "application/json;charset=utf-8"}
    },
    cpr::Body{payload.dump()}
  );
  auto status_code = r.status_code;
  auto response_text = r.text;
}
} // namespace request

namespace translation {
std::string ExtractSubtext(std::string text) {
 return "";
}

std::string TranslateText(std::string text) {
  // TODO: Replace no-op
  request::MakeRequest(text);
  return text;
}
} // namespace translation

std::string TranslateToKorean(std::string text) {
  return translation::TranslateText(
    translation::ExtractSubtext(
      text
    )
  );
}
} // namespace korean