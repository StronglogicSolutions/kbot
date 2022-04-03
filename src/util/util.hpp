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

static void SaveToFile(const std::string& data, const std::string& path)
{
  std::ofstream o{path};
  o << data;
}

[[ maybe_unused ]]
static std::string ExtractTempFilename(const std::string& full_url)
{
  static const char* TEMP_FILE{"temp_file"};
  auto pos   = full_url.find_last_of("/");
  std::string name = (pos == std::string::npos) ? full_url : full_url.substr(pos + 1);

        auto ext_end  = full_url.find_first_of('?');
             ext_end  = ext_end == std::string::npos ? full_url.size() : ext_end;
  const auto url      = full_url.substr(0, ext_end);
  const auto ext_beg  = url.find_last_of('.');
  const auto ext_len  = (ext_beg != url.npos) ? (url.size() - ext_beg) : 0;
  const auto filename = (ext_len > 0) ? name + url.substr(ext_beg, ext_len) : name;
  return filename;
}

[[ maybe_unused ]]
static std::string FetchTemporaryFile(const std::string& full_url, const bool verify_ssl = true)
{
  const auto filename   = ExtractTempFilename(full_url);
  const cpr::Response r = cpr::Get(cpr::Url{full_url}, cpr::VerifySsl(verify_ssl));
  SaveToFile(r.text, filename);

  return filename;
}

} // namespace kbot