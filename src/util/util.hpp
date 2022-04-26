#pragma once

#include <ctime>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cpr/cpr.h>
namespace kbot {
/**
 * Poor man's log
 */
template<typename... Args>
static void log(Args... args)
{
  for (const auto& s : { args... }) std::cout << s;
  std::cout << std::endl;
}

/**
 * SanitizeJSON
 *
 * Helper function to remove escaped double quotes from a string
 *
 * @param [in] {std::string&} A reference to a string object
 */
inline std::string UnescapeQuotes(std::string s) {
  s.erase(
    std::remove(s.begin(), s.end(),'\"'),
    s.end()
  );

  return s;
}


inline const std::time_t to_unixtime(const char* datetime) {
  std::tm            t{};
  std::istringstream ss{datetime};

  ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");

  return mktime(&t);
}

static std::string RemoveSlashes(std::string s)
{
  s.erase(std::remove(s.begin(), s.end(), '\\'), s.end());

  return s;
}

static void save_as_file(const std::string& data, const std::string& path)
{
  std::ofstream o{path};
  o << data;
}

[[ maybe_unused ]]
static std::string ExtractFilename(const std::string& full_url)
{
  auto FindProt  = []         (const auto& s) { auto i = s.find_last_of("://"); return (i != s.npos) ? i + 1 : 0; };
  auto FindQuery = []         (const auto& s) { auto i = s.find_first_of('?');  return (i != s.npos) ? i : 0;     };
  auto FindExt   = []         (const auto& s) { auto i = s.find_last_of('.');   return (i != s.npos) ? i : 0;     };
  auto SimpleURL = [FindQuery](const auto& s) {                                 return s.substr(0, FindQuery(s)); };
  auto Filename  = [SimpleURL, FindProt, FindExt](const auto& full_url)
  {
    const auto s_url     = SimpleURL(full_url);
    const auto url       = (s_url.empty()) ? full_url : s_url;
    const auto uri       = url.substr(FindProt(url));
    const auto ext       = FindExt(uri);
    const auto sub_uri   = (ext) ? uri.substr(0, ext) : uri;
    const auto extension = uri.substr(ext);
    const auto sub_ext   = FindExt(sub_uri);
    return (sub_ext) ? sub_uri.substr(sub_ext) + extension : sub_uri + extension;
  };

  return Filename(full_url);
}

[[ maybe_unused ]]
static std::string FetchTemporaryFile(const std::string& full_url, const bool verify_ssl = true)
{
  const auto filename   = ExtractFilename(full_url);
  const cpr::Response r = cpr::Get(cpr::Url{full_url}, cpr::VerifySsl(verify_ssl));
  save_as_file(r.text, filename);

  return filename;
}

} // namespace kbot