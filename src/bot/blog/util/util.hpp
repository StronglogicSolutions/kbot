#include <string>
#include <cpr/cpr.h>
#include <fstream>
#include <algorithm>

namespace kiq::kbot
{
static void SaveToFile(const std::string& data, const std::string& path)
{
  std::ofstream o{path};
  o << data;
}
//-----------------------------------------------------------------------
static bool is_local(std::string_view s)
{
  return s.find("file://") != s.npos;
}
//-----------------------------------------------------------------------
static const std::string FetchFile(const std::string& full_url, const std::string& path, const bool verify_ssl = true)
{
  auto sanitz_s = [](auto s) { s.erase(std::remove(s.begin(), s.end(), '\\'), s.end()); return s; };
  auto strpprot = [](const auto url) { return url.substr(url.find("://") + 3); };

        auto s_url   = sanitz_s(full_url);
        auto ext_end = s_url.find_first_of('?');
             ext_end = ext_end == std::string::npos ? s_url.size() : ext_end;
  const auto url     = s_url.substr(0, ext_end);
  if (is_local(url))
  {
    const auto file_path = strpprot(url);
    const auto filename  = url.substr(url.find_last_of('/') + 1);
    std::system(std::string{"cp " + file_path + " " + path + "/" + filename}.c_str());
    return filename;
  }

  const auto ext_beg = url.find_last_of('.');
  const auto ext_len = (ext_beg != url.npos) ? (url.size() - ext_beg) : 0;
  if (const auto fln_beg = url.find_last_of('/'); fln_beg != std::string::npos)
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
} // ns kiq::kbot
