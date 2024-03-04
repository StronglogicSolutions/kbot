#pragma once

#include <ctime>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cpr/cpr.h>
#include <logger.hpp>
namespace kiq
{
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
  auto Filename  = [](const auto& full_url)
  {
    auto FindProt  = []         (const auto& s) { auto i = s.find_last_of("://"); return (i != s.npos) ? i + 1 : 0; };
    auto FindQuery = []         (const auto& s) { auto i = s.find_first_of('?');  return (i != s.npos) ? i : 0;     };
    auto FindExt   = []         (const auto& s) { auto i = s.find_last_of('.');   return (i != s.npos) ? i : 0;     };
    auto SimpleURL = [FindQuery](const auto& s) {                                 return s.substr(0, FindQuery(s)); };
    auto clean_str = []         (      auto& s) { s.erase(remove_if(s.begin(), s.end(), [](char c)
                                                { return !(c>=0 && c <128); }), s.end()); };
    const auto s_url     = SimpleURL(full_url);
    const auto url       = (s_url.empty()) ? full_url : s_url;
    const auto uri       = url.substr(FindProt(url));
    const auto ext       = FindExt(uri);
    const auto sub_uri   = (ext) ? uri.substr(0, ext) : uri;
    const auto extension = uri.substr(ext);
    const auto sub_ext   = FindExt(sub_uri);
          auto ret_s     = (sub_ext) ? sub_uri.substr(sub_ext) + extension : sub_uri + extension;
    clean_str(ret_s);
    return ret_s;
  };

  return Filename(full_url);
}

[[ maybe_unused ]]
static std::string FetchTemporaryFile(const std::string& full_url, const bool verify_ssl = true)
{
  const auto filename   = ExtractFilename(full_url);
  const cpr::Response r = cpr::Get(cpr::Url{full_url});//, cpr::VerifySsl(verify_ssl));
  save_as_file(r.text, filename);

  return filename;
}

//-----------------------------------------------------------------------
using paths_t = std::vector<std::string>;
static paths_t FetchFiles(const paths_t& urls, const std::string& dir = "")
{
  paths_t fetched;
  for (const auto& url : urls) if (!url.empty()) fetched.push_back(dir + '/' + kbot::FetchTemporaryFile(url));
  return fetched;
}
//-----------------------------------------------------------------------
template <int64_t INTERVAL = 3000>
class timer
{
using time_point_t = std::chrono::time_point<std::chrono::system_clock>;
using ms_t         = std::chrono::milliseconds;
using duration_t   = std::chrono::duration<std::chrono::system_clock, ms_t>;

public:
  bool
  check_and_update()
  {
    if (const auto tp = now(); ready(tp))
    {
      _last = tp;
      return true;
    }
    return false;
  }

private:
  bool
  ready(const time_point_t t) const
  {
    return (std::chrono::duration_cast<ms_t>(t - _last).count() > INTERVAL);
  }
//-----------------------------------------------------------------------
  time_point_t
  now()
  {
    return std::chrono::system_clock::now();
  }
//-----------------------------------------------------------------------
  time_point_t _last{now()};
};
} // namespace kbot
} // ns kiq