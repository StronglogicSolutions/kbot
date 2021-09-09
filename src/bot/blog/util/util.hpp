#include <string>
#include <cpr/cpr.h>
#include <fstream>

namespace kbot
{
static void SaveToFile(const std::string& data, const std::string& path)
{
  std::ofstream o{path};
  o << data;
}
static const std::string FetchFile(const std::string& full_url, const std::string& path, const bool verify_ssl = true)
{
  auto ext_end = full_url.find_first_of('?');
       ext_end = ext_end == std::string::npos ? full_url.size() : ext_end;
  const auto url     = full_url.substr(0, ext_end);
  const auto ext_beg = url.find_last_of('.');
  const auto ext_len = (ext_beg != url.npos) ? (url.size() - ext_beg) : 0;
  const auto fln_beg = url.find_last_of('/');

  if (fln_beg != std::string::npos)
  {
    const std::string   filename = url.substr(fln_beg + 1);
    const cpr::Response r        = cpr::Get(cpr::Url{full_url}, cpr::VerifySsl(verify_ssl));
    if (r.error.code == cpr::ErrorCode::OK)
    {
      SaveToFile(r.text, filename);
      return filename;
    }
  }

  return "";
}
} // ns kbot
