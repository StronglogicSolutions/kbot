#include "youtube.hpp"

namespace kbot {

const std::string DEFAULT_API_NAME{"YouTube Data API"};
const std::string DEFAULT_USERNAME{"@Emmanuel Buckshi"};

/**
  * @constructor
 */

YouTubeBot::YouTubeBot()
: Bot("YouTubeBot"),
  m_has_promoted(false),
  m_is_own_livestream(false),
  m_time_value(clock()),
  m_nlp(NLP{DEFAULT_USERNAME})
{}

void YouTubeBot::Init() {
  init();
}

/**
 * init
 */
bool YouTubeBot::init() {
  if (!m_api.is_authenticated() && !m_api.init())
    throw std::runtime_error{"Could not authenticate YouTube API"};
  if (!m_api.FetchLiveVideoID().empty()) {
    if (m_api.FetchLiveDetails())
    {
      m_api.FetchChatMessages();

      if (m_api.GreetOnEntry()) {
        // m_api.PostMessage("Hello");
      }
      return true;
    }
  }
  return false;
}

/**
 * loop
 *
 * The loop method runs on its own thread
 */
void YouTubeBot::loop() {
  using namespace korean;

  uint8_t no_hits{0};
  static std::chrono::time_point<std::chrono::system_clock> initial_time = std::chrono::system_clock::now();

  while (m_is_running) {
    log("YouTubeBot alive");

    const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - initial_time).count();

    if (!m_api.GetLiveDetails().id.empty())
    {
      if (elapsed > 1800)
      {
        PostMessage("If you like this type of content, smash the LIKE and SHARE!! :)");
        initial_time = now;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }
}

  /**
   * CreateReplyMessages
   */
std::vector<std::string> YouTubeBot::CreateReplyMessages(LiveMessages messages, bool bot_was_mentioned) {
  std::vector<std::string> reply_messages{};
  // if (bot_was_mentioned) {
  auto reply_number = (!m_has_promoted) ? messages.size() + 1 : messages.size();
  reply_messages.reserve(reply_number); // Create responses to all who mentioned bot specifically

  for (const auto& message : messages) {
    auto user_id = message.author;
    if (!message.tokens.empty()) {
      for (const auto& token : message.tokens) {
        /**
          ┌───────────────────────────────────────────────────────────┐
          │░░░░░░░░░░░░░░░░░░░░░░TRACKING REPLIES░░░░░░░░░░░░░░░░░░░░░░│
          │░░░░░░░░Track if we have already responded to the author:░░░░░│
          │░░░░░░░░1. For their location                            ░░░░░│
          │░░░░░░░░2. To greet them                                 ░░░░░│
          │░░░░░░░░3. For a particular mention                      ░░░░░│
          └───────────────────────────────────────────────────────────┘
          */

        if (token.type == TokenType::location //&&
            //!api.HasInteracted(user_id, Interaction::location_ask)
        ) {
          reply_messages.push_back(CreateLocationResponse(token.value));
          m_api.RecordInteraction(message.author, Interaction::location_ask, token.value);
        }
        else
        if (token.type == TokenType::person //&&
                  //!api.HasInteracted(user_id, Interaction::greeting)
        ) {
          reply_messages.push_back(CreatePersonResponse(token.value));
          m_api.RecordInteraction(message.author, Interaction::greeting, token.value);
        }
        else
        if (token.type == TokenType::organization //&&
                  //!api.HasInteracted(user_id, Interaction::probing)
        ) {
          reply_messages.push_back(CreateOrganizationResponse(token.value));
          m_api.RecordInteraction(message.author, Interaction::probing, token.value);
        }

        m_nlp.Insert(
            std::move(Message{
                .text = reply_messages.back(), .received = false}),
            message.author,
            token.value);
      }
    }
  }
  // } else {
  //   reply_messages.reserve((!m_has_promoted) ? 2 : 1);
  //   reply_messages.push_back("Hello from the KBot!");
  // }

  if (!m_has_promoted) {
    reply_messages.push_back(CreatePromoteResponse());
    m_has_promoted = true;
  }

  return reply_messages;
}

  /**
   * GetAPI
   *
   * @param
   * @returns
   */
std::unique_ptr<API> YouTubeBot::GetAPI(std::string name) {
  return nullptr;
}

/**
 * GetChats
 *
 * @returns [out] {LiveChatMap}  A map of Live Chats indexed by chat id
 */
LiveChatMap YouTubeBot::GetChats() {
    return m_api.GetChats();
}


/**
 * PostMessage
 *
 * @param   [in]  {std::string}
 * @returns [out] {bool}
 */
bool YouTubeBot::PostMessage(std::string message) {
  m_posted_messages.push_back(message);
  return m_api.PostMessage(message);
}

/**
 *
 */
std::string YouTubeBot::GetResults() {
  std::string result{};
  try {
    for (const auto m : m_nlp.GetConversations()) {
      auto interlocutor = m.first;
      auto subject      = m.second->subjective->toString();
      auto message      = m.second->text;
      auto was_received = m.second->received;

      result += "Interlocutor: " + interlocutor +
                "\nSubjects: " + subject +
                "\nMessage: " + message + "\n";
    }
  } catch (const std::exception e) {
    std::cout << "Exception caught: " << e.what() << std::endl;
  }
  return result;
}

void YouTubeBot::SetCallback(BrokerCallback cb_fn) {
  m_send_event_fn = cb_fn;
}

bool YouTubeBot::HandleEvent(BotEvent event) {
  if (event.name == "youtube:livestream")
  {
    VideoDetails video_info = m_api.GetLiveDetails();

    std::string name = (!video_info.id.empty()) ?
                         "livestream active" :
                         "livestream inactive";
    std::string payload{
      video_info.channel_title + " currently has a livestream RIGHT NOW!!\n" +
      video_info.url           + '\n' +
      video_info.title         + '\n'
    };

    for (const auto& platform : std::vector<Platform>{Platform::mastodon, Platform::discord})
      m_send_event_fn(
        BotEvent{
          .platform = platform,
          .name = name,
          .data = payload,
          .urls = (video_info.thumbnail.empty()) ?
                    std::vector<std::string>{} :
                    std::vector<std::string>{video_info.thumbnail}
        }
      );
  }

  return true;
}

bool YouTubeBot::IsRunning() {
  return m_is_running;
}

void YouTubeBot::Start()
{
  if (!m_is_running)
    Worker::start();
}

void YouTubeBot::Shutdown() {
  Worker::stop();
}

} // namespace kbot
