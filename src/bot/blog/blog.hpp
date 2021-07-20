#pragma once

#include "interfaces/interfaces.hpp"
#include <fstream>

namespace kbot {
namespace constants {
const std::string USER{""};
} // namespace constants

static std::string GetBlogPath()
{
  // return INIReader{GetConfigPath()};
  return "";
}

static std::string get_simple_datetime() {
  uint8_t            buffer_size{24};
  char               b[buffer_size];
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  struct tm tm{};
  if (::gmtime_r(&now, &tm))
    if (std::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%S", &tm))
      return std::string{b};
  throw std::runtime_error{"Failed to get current date as string"};
}

static std::vector<std::string> FindTags(const std::string& s)
{
  std::vector<std::string> tags{};

  auto start_it = s.find('#');
  while (start_it != std::string::npos)
  {
    auto end_it = s.substr(start_it).find_first_of(' ');
    if (end_it == std::string::npos)
      break;
    tags.emplace_back(s.substr(start_it, (end_it - start_it)));
  }

  log("Tags found\n");

  for (const auto& tag : tags)
    log(tag);

  return tags;
}

static std::string CreateMarkdownImage(const std::string& url)
{
  return "!['Blog Image'](" + url + " 'Blog image')\n";
}

static std::string CreateBlogPost(const std::string&              text,
                                  const std::vector<std::string>& tags,
                                  const std::vector<std::string>& urls)
{
  std::string       delim = "";
  const auto        end_it= text.find_first_of('\n');
  const std::string title = (end_it != std::string::npos) ?
                              text.substr(0, text.size() - end_it) :
                              "Platform repost";
  std::string blog_post{};
  blog_post += "---\n";
  blog_post += "title: " + title                 + '\n';
  blog_post += "date: "  + get_simple_datetime() + '\n';
  blog_post += "tags: ";
  for (const auto& tag : tags)
  {
    blog_post += delim + tag;
    delim = ',';
  }
  blog_post += '\n';
  blog_post += "---\n";
  blog_post += text;
  blog_post += '\n';
  for (const auto& url : urls) blog_post += CreateMarkdownImage(url);

  return blog_post;
}

class BlogBot : public kbot::Worker,
                   public kbot::Bot
{
public:
BlogBot()
: kbot::Bot{constants::USER}
{}

virtual void Init() override
{
  return;
}

virtual void loop() override
{
  while (m_is_running)
  {
    std::cout << "BlogBot alive" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }
}



virtual void SetCallback(BrokerCallback cb_fn) override
{
  m_send_event_fn = cb_fn;
}


virtual bool HandleEvent(BotRequest request) override
{
  const auto PostBlog = [&](const std::string text, const std::vector<std::string> media_urls) -> bool
  {
    const std::vector<std::string> tags = FindTags(text);
    const std::string blog_post         = CreateBlogPost(text, tags, media_urls);

    std::ofstream out{};
    out << blog_post;
    return false;
  };

  const auto event = request.event;

  (request.data); // text
  (request.urls); // media

  /**
   * TODO: HtmlBuilder?
   *
   */

  const bool error = (!PostBlog(request.data, request.urls));
  if (error)
  {
    std::string error_message{"Failed to handle " + request.event};
    kbot::log(error_message);
    m_send_event_fn(CreateErrorEvent(error_message, request));
  }
  else
    m_send_event_fn(CreateSuccessEvent(request));

  return (!error);
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
