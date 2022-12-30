#include <string>
#include <cpr/cpr.h>
#include <fstream>
#include <algorithm>

namespace kbot
{
static void SaveToFile(const std::string& data, const std::string& path)
{
  std::ofstream o{path};
  o << data;
}
static const std::string FetchFile(const std::string& full_url, const std::string& path, const bool verify_ssl = true)
{
  const auto SanitizeString = [](std::string s) -> std::string
  {
    const auto target{"\\"};
    s.erase(std::remove(s.begin(), s.end(), '\\'), s.end());
    return s;
  };
  auto s_url   = SanitizeString(full_url);
  auto ext_end = s_url.find_first_of('?');
       ext_end = ext_end == std::string::npos ? s_url.size() : ext_end;
  const auto url     = s_url.substr(0, ext_end);
  const auto ext_beg = url.find_last_of('.');
  const auto ext_len = (ext_beg != url.npos) ? (url.size() - ext_beg) : 0;
  const auto fln_beg = url.find_last_of('/');

  if (fln_beg != std::string::npos)
  {
    const std::string   filename = url.substr(fln_beg + 1);
    const cpr::Response r        = cpr::Get(cpr::Url{s_url});
    if (r.error.code == cpr::ErrorCode::OK)
    {
      SaveToFile(r.text, (path + filename));
      return filename;
    }
  }

  return "";
}
} // ns kbot
