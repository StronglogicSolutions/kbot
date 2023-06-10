#pragma once

#include "interfaces/interfaces.hpp"
#include "util/util.hpp"
#include "INIReader.h"
#include <fstream>

namespace kbot {
namespace constants::blog {
const std::string USER{""};
const uint8_t     APP_NAME_LENGTH{6};
const std::string DEFAULT_CONFIG_PATH{"config/config.ini"};
} // ns constants::blog

static std::string get_executable_cwd()
{
  std::string full_path{realpath("/proc/self/exe", NULL)};
  return full_path.substr(0, full_path.size() - (constants::blog::APP_NAME_LENGTH  + 1));
}

static const std::string GetConfigPath()
{
  return get_executable_cwd() + "/../" + constants::blog::DEFAULT_CONFIG_PATH;
}

static std::string GetBlogPath()
{
  const auto        config_path = GetConfigPath();
  const auto        config      = INIReader{config_path};
  std::string       path        = config.GetString("blog_bot", "post_path", "");
  if (!path.empty() && path.back() != '/')
    path += '/';
  return path;
}

static std::string GetBlogImagePath()
{
  const auto        config_path = GetConfigPath();
  const auto        config      = INIReader{config_path};
  const std::string path        = config.GetString("blog_bot", "image_path", "");
  return path;
}

static std::string unixtime()
{
  return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()
  ).count());
}

static std::string get_simple_datetime()
{
  uint8_t            buffer_size{24};
  char               b[buffer_size];
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  struct tm tm{};
  if (::gmtime_r(&now, &tm))
    if (std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &tm))
      return std::string{b};
  throw std::runtime_error{"Failed to get current date as string"};
}

static std::vector<std::string> FindTags(const std::string& s)
{
  std::vector<std::string> tags{};
  std::string              t_s{s};

  for (auto start_it = t_s.find('#'); start_it != std::string::npos; start_it = t_s.find('#'))
  {
    const auto chunk  = t_s.substr(start_it + 1);
    const auto s_it   = chunk.find_first_of(' ');
    const auto l_it   = chunk.find_first_of('\n');
    const auto end_it = (s_it < l_it) ? s_it : l_it;
    const bool done   = (end_it == std::string::npos);
    t_s               = (done) ? "" : t_s.substr((start_it + end_it + 1));
    tags.emplace_back((end_it == std::string::npos) ? chunk : chunk.substr(0, end_it));
  }

  return tags;
}

static std::string CreateMarkdownImage(const std::string& url)
{
  return "!['Blog Image'](" + url + " 'Blog image')\n";
}

struct BlogPost
{
std::string title;
std::string text;
};

static BlogPost CreateBlogPost(const std::string&              text,
                                  const std::vector<std::string>& tags,
                                  const std::vector<std::string>& urls)
{
  const auto MakeTags = [](const std::vector<std::string>& tags) -> const std::string
  {
    std::string s{};
    for (const auto& tag : tags) s += "- " + tag + '\n';
    return s;
  };
  std::string       delim = "";
  const auto        end_it= text.find_first_of('\n');
  const std::string title = (end_it != std::string::npos) ?
                              text.substr(0, (end_it - 1)) :
                              "Platform repost";
  std::string blog_post{};
  blog_post += "---\n";
  blog_post += "title: "  + title                 + '\n';
  blog_post += "date: "   + get_simple_datetime() + '\n';
  blog_post += "tags: \n" + MakeTags(tags);
  blog_post += '\n';
  blog_post += "---\n";
  for (const auto& url : urls)
  {
    const auto& filename = FetchFile(url, GetBlogImagePath());
    if (!filename.empty())
      blog_post += CreateMarkdownImage((GetBlogImagePath() + '/' + filename)) + '\n';
  }
  blog_post += text;
  blog_post += '\n';

  return BlogPost{.title = title, .text = blog_post};
}

class BlogBot : public kbot::Worker,
                public kbot::Bot
{
public:
BlogBot()
: kbot::Bot{constants::blog::USER}
{}

virtual void Init(bool flood_protect) final
{
  return;
}

virtual void loop() override
{
  while (m_is_running)
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}

virtual void SetCallback(BrokerCallback cb_fn) override
{
  m_send_event_fn = cb_fn;
}

virtual bool HandleEvent(const BotRequest& request) override
{
  static const auto MAX_TITLE_SIZE{25};

  const auto PostBlog = [&](const std::string text, const std::vector<std::string> media_urls) -> bool
  {
    const auto     tags      = FindTags(text);
    const BlogPost blog_post = CreateBlogPost(text, tags, media_urls);
    const auto     title     = (blog_post.title.size() > MAX_TITLE_SIZE) ?
                                blog_post.title.substr(0, MAX_TITLE_SIZE) : blog_post.title;
    const auto     filename  = GetBlogPath() + title + '_' + unixtime() + ".md";


    std::ofstream                  out{filename};
    if (out << blog_post.text)
      return true;
    return false;
  };

  if (request.event == "platform:repost")
    if (PostBlog(request.data, request.urls))
    {
      klog().i("Succesfully created a new post");
      m_send_event_fn(CreateSuccessEvent(request));
      return true;
    }
    else
      m_send_event_fn(CreateErrorEvent("Failed to post blog", request));
  klog().e("Failed to handle post: {}", request.id);
  return false;
}

virtual std::unique_ptr<API> GetAPI(std::string name) override
{
  // TODO: Add other APIs
  return nullptr;
}

virtual bool IsRunning() override
{
  return m_is_running;
}

virtual void Start() override
{
  if (!m_is_running)
    Worker::start();
}

virtual void Shutdown() override
{
  Worker::stop();
}

private:
BrokerCallback m_send_event_fn;

};
} // namespace kbot
