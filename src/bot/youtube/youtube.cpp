#include "youtube.hpp"
#include "constants.hpp"

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
  m_nlp(DEFAULT_USERNAME),
  m_db(DatabaseConfiguration{
    DatabaseCredentials{
      .user = ktube::GetConfigReader().GetString(
        constants::YOUTUBE_DB_SECTION,
        constants::YOUTUBE_DB_USER,
        ""),
      .password = ktube::GetConfigReader().GetString(
        constants::YOUTUBE_DB_SECTION,
        constants::YOUTUBE_DB_PASS,
        ""),
      .name = ktube::GetConfigReader().GetString(
        constants::YOUTUBE_DB_SECTION,
        constants::YOUTUBE_DB_NAME,
        "")
    },
    "127.0.0.1",
"5432"
    }
  )
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

  while (IsRunning()) {
    const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - initial_time).count();
    log("YouTubeBot alive. Elapsed: " + std::to_string(elapsed));

    if (m_api.HasChats() && elapsed > 1800)
    {
      PostMessage("If you enjoy this content please SMASH the like and Share!");
      initial_time = now;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }
}

void YouTubeBot::UpdateChats()
{
  m_api.ParseTokens();

  for (const auto& reply : CreateReplyMessages(m_api.GetCurrentChat()))
    PostMessage(reply);

  kbot::log(m_nlp.toString());

}

  /**
   * CreateReplyMessages
   */
std::vector<std::string> YouTubeBot::CreateReplyMessages(LiveMessages messages, bool bot_was_mentioned) {
  using namespace conversation;
  using namespace ktube;
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

/**
 * @brief
 *
 * @param event
 * @return true
 * @return false
 */
bool YouTubeBot::HandleEvent(BotEvent event) {
  using namespace ktube;
  bool error{false};

  if (event.name == "youtube:livestream")
  {
    VideoDetails video_info = m_api.GetLiveDetails();

    auto has_livestream = (!video_info.id.empty());

    std::string name = (has_livestream) ?
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
  else
  if (event.name == "youtube:comment")
  {
    using namespace ktube;
    // std::string reply_text{"외국인과 언어교환하고 싶은 분은 여기로 방문해 주세요 👉 https://discord.gg/j5Rjhk96"};

    const std::string& reply_text           = event.data;
    const std::vector<std::string> keywords = event.urls; // TODO: rename `urls` to `args`

    bool comment_result{false};
    bool reply_result{false};

    for (const auto& video : m_api.fetch_rival_videos(ktube::Video::CreateFromTags(keywords)))
    {
      if (HaveCommented(video.id) || video.id == "E0tuY6yV3CQ" || video.id == "yFljsR5dEos")
        continue;

      Comment comment  = Comment::Create(reply_text);
      comment.video_id = video.id;
      comment.channel  = video.channel_id;
      comment.id       = m_api.PostComment(comment);

      if (!comment.id.empty())
      {
        InsertComment(comment);
        for (const auto& comment : m_api.FetchVideoComments(video.id))
        {
          if (HaveReplied(comment.id))
            continue;

          Comment reply_comment   = Comment::Create(reply_text);
          reply_comment.parent_id = comment.id;
          reply_comment.video_id  = video.id;
          reply_comment.channel   = video.channel_id;

          reply_comment.id = m_api.PostCommentReply(reply_comment);

          if (!reply_comment.id.empty())
            InsertComment(reply_comment);

          break;
        }
        break;
      }
    }
    error = comment_result;
  }

  return (!error);
}

/**
 * @brief InsertComment
 *
 * @param comment
 * @return true
 * @return false
 */
bool YouTubeBot::InsertComment(const ktube::Comment& comment)
{
  std::string channel_id{};
  std::string video_id{};
  Values      values{};
  Fields      fields{};

  for (const auto& value : m_db.select("channel", {"id"}, {{"cid", comment.channel}}))
    if (value.first == "id")
      channel_id = value.second;

  channel_id = (channel_id.empty()) ?
                 m_db.insert("channel", {"cid"}, {comment.channel}, "id") :
                 channel_id;

  if (m_db.select("video", {"id"}, {{"vid", comment.video_id}}).empty())
    m_db.insert("video", {"vid", "chid"}, {comment.video_id, channel_id}, "id");

  if (!comment.parent_id.empty())
  {
    std::string parent_comment_id{};
    for (const auto& row : m_db.select("comment", {"id"}, {{"comment_id", comment.parent_id}}))
    {
      if (row.first == "id")
      {
        parent_comment_id = row.second;
        break;
      }
    }
    parent_comment_id = (parent_comment_id.empty()) ?
                          m_db.insert("comment", {"vid", "comment_id"}, {comment.video_id, comment.parent_id}, "id") :
                          parent_comment_id;
    fields = Fields{"vid", "comment_id", "parent_id"};
    values = Values{comment.video_id, comment.id, parent_comment_id};
  }
  else
  {
    fields = Fields{"vid", "comment_id"};
    values = Values{comment.video_id, comment.id};
  }

  return (!m_db.insert("comment", fields, values, "id").empty());

}
/**
 * @brief HaveCommented
 *
 * @param   [in] {std::string} vid  The video ID
 * @returns [out] {bool}            Indicating whether a top-level comment has been posted
 */
bool YouTubeBot::HaveCommented(const std::string& vid)
{
  return !m_db.selectSimpleJoin<CompJoinFilter>(
    "video",
    {
      "comment.id"
    },
    {
      CompJoinFilter{
        { "comment.parent_id", "NULL", "is"},
        { "comment.vid",       vid,     "="}
      }
    },
    Join{
      .table      ="comment",
      .field      ="vid",
      .join_table ="video",
      .join_field ="vid",
      .type       = JoinType::INNER
    }
  ).empty();
}

/**
 * @brief HaveReplied
 *
 * @param   [in] {std::string} cid The comment ID
 * @returns [out] {bool}           Indicating whether a comment has been replied to
 */
bool YouTubeBot::HaveReplied(const std::string& cid)
{
  return !m_db.selectSimpleJoin(
    "comment as c",
    {
      "comment.id"
    },
    {
      QueryFilter{
        { "comment.comment_id", cid},
      }
    },
    Join{
      .table      ="comment",
      .field      ="id",
      .join_table ="c",
      .join_field ="parent_id",
      .type       = JoinType::INNER
    }
  ).empty();
}

bool YouTubeBot::IsRunning() {
  return Worker::m_is_running;
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
