#include "youtube_api.hpp"


namespace kbot {
const std::string CreateLocationResponse(std::string location) {
  return std::string{
    "Cool to see someone from " + location + ". How is life treating you there?"
  };
}

const std::string CreatePersonResponse(std::string name) {
  return std::string{
    "Hello, " + name + ". How are you?"
  };
}

const std::string CreateOrganizationResponse(std::string name) {
  return std::string{
    "I will need to familiarize myself better with " + name
  };
}

const std::string CreatePromoteResponse(bool test_mode) {
  if (test_mode) {
    return constants::promotion::support;
  }

  return constants::promotion::test_support;
}

std::string to_youtube_url(const std::string& id)
{
  return "https://www.youtube.com/watch?v=" + id;
}


YouTubeDataAPI::YouTubeDataAPI ()
: m_greet_on_entry(false),
  m_test_mode(false),
  m_retry_mode(false) {
  INIReader reader{constants::DEFAULT_CONFIG_PATH};

  if (reader.ParseError() < 0) {
    log("Error loading config");
  }

  auto youtube_key = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_KEY, "");
  if (!youtube_key.empty()) {
    m_auth.key = youtube_key;
  }

  auto app_path = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_TOKEN_APP, "");
  if (!app_path.empty()) {
    m_auth.token_app_path = app_path;
  }

  auto username = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_USERNAME, "");
  if (!username.empty()) {
    m_username = username;
  }

  auto greet_on_entry = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_GREET, "");
  if (!greet_on_entry.empty()) {
    m_greet_on_entry = greet_on_entry.compare("true") == 0;
  }

  auto test_mode = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_TEST_MODE, "");
  if (!test_mode.empty()) {
    m_test_mode = test_mode.compare("true") == 0;
  }

  auto retry_mode = reader.GetString(constants::YOUTUBE_CONFIG_SECTION, constants::YOUTUBE_RETRY_MODE, "");
  if (!retry_mode.empty()) {
    m_retry_mode = retry_mode.compare("true") == 0;
  }

  if (m_auth.token_app_path.empty() || m_auth.key.empty()) {
    throw std::invalid_argument{"Cannot run YouTube API without key and token app"};
  }
}

  /**
   * GetType
   *
   * Returns the name of this API
   */
  std::string YouTubeDataAPI::GetType() {
    return std::string{"YouTube Data API"};
  }

  /**
   * GetBearerAuth
   *
   * @returns [out] {std::string}
   */
  std::string YouTubeDataAPI::GetBearerAuth() {
    if (m_auth.access_token.empty()) return "";
    return std::string{"Bearer " + m_auth.access_token};
  }

  /**
   * FetchToken
   *
   * @returns [out] {std::string}
   */
  std::string YouTubeDataAPI::FetchToken() {
    if (m_auth.access_token.empty()) {

      ProcessResult result = qx({m_auth.token_app_path});

      if (result.error) {
        std::cout << "Error executing program to retrieve token" << std::endl;
        return "";
      }

      json auth_json = json::parse(result.output);

      if (auth_json.empty()) {
        return "";
      }

      if (!auth_json.is_null() && auth_json.is_object()) {
        m_auth.access_token = auth_json["access_token"].dump();
        m_auth.scope        = auth_json["scope"].dump();
        m_auth.token_type   = auth_json["token_type"].dump();
        m_auth.expiry_date  = auth_json["expiry_date"].dump();

        log("Fetched token successfully");
      }
    }

    return m_auth.access_token;
  }

  /**
   * GetAuth
   *
   * @returns [out] {AuthData}
   */
  AuthData YouTubeDataAPI::GetAuth() {
    return m_auth;
  }

  /**
   * FetchLiveVideoID
   *
   * @returns [out] {std::string}
   */
  std::string YouTubeDataAPI::FetchLiveVideoID() {
    using namespace constants;
    if (m_auth.access_token.empty())
      throw std::invalid_argument("Cannot use YouTube API without access token");

    cpr::Response r = cpr::Get(
      cpr::Url{URL_VALUES.at(SEARCH_URL_INDEX)},
      cpr::Header{
        {HEADER_NAMES.at(ACCEPT_HEADER_INDEX), HEADER_VALUES.at(APP_JSON_INDEX)},
        {HEADER_NAMES.at(AUTH_HEADER_INDEX),   GetBearerAuth()}
      },
      cpr::Parameters{
        {PARAM_NAMES.at(PART_INDEX),    PARAM_VALUES.at(SNIPPET_INDEX)},
        {PARAM_NAMES.at(KEY_INDEX),     m_auth.key},
        {PARAM_NAMES.at(CHAN_ID_INDEX), PARAM_VALUES.at(SL_CHAN_KEY_INDEX)},
        {PARAM_NAMES.at(EVENT_T_INDEX), PARAM_VALUES.at(LIVE_EVENT_TYPE_INDEX)},
        {PARAM_NAMES.at(TYPE_INDEX),    PARAM_VALUES.at(VIDEO_TYPE_INDEX)}
      }
    );

    json video_info = json::parse(r.text);

    if (!video_info.is_null() && video_info.is_object()) {
      auto items = video_info["items"];
      if (!items.is_null() && items.is_array() && items.size() > 0) {
        log(items[0]["snippet"].dump());
        m_video_details.id = items[0]["id"]["videoId"];
        SanitizeJSON(m_video_details.id);
        m_video_details.title         = items[0]["snippet"]["title"];
        m_video_details.description   = items[0]["snippet"]["description"];
        m_video_details.channel_title = items[0]["snippet"]["channelTitle"];
        m_video_details.channel_id    = items[0]["snippet"]["channelId"];
        m_video_details.url           = to_youtube_url(m_video_details.id);
        m_video_details.thumbnail     = (items[0]["snippet"].contains("thumbnails") &&
                                        !items[0]["snippet"]["thumbnails"].is_null()) ?
                                        items[0]["snippet"]["thumbnails"]["high"]["url"] :
                                        "";
      }
      log("Fetched live video details for channel " + PARAM_VALUES.at(SL_CHAN_KEY_INDEX));
    }

    return m_video_details.id;
  }

  /**
   * GetLiveDetails
   *
   * @returns [out] {VideoDetails}
   */
  VideoDetails YouTubeDataAPI::GetLiveDetails() {
    return m_video_details;
  }

  /**
   * FetchLiveDetails
   *
   * @returns [out] {bool}
   */
  bool YouTubeDataAPI::FetchLiveDetails() {
    using namespace constants;
    if (m_auth.access_token.empty())
      throw std::invalid_argument{"Unable to use YouTubeDataAPI without token"};

    if (m_video_details.id.empty()) {
      log("Unable to Fetch live details: no video ID");
      return false;
    }

    cpr::Response r = cpr::Get(
      cpr::Url{URL_VALUES.at(VIDEOS_URL_INDEX)},
      cpr::Header{
        {HEADER_NAMES.at(ACCEPT_HEADER_INDEX), HEADER_VALUES.at(APP_JSON_INDEX)},
        {HEADER_NAMES.at(AUTH_HEADER_INDEX),   GetBearerAuth()}
      },
      // ,
      cpr::Parameters{
        {PARAM_NAMES.at(PART_INDEX),    PARAM_VALUES.at(LIVESTREAM_DETAILS_INDEX)},
        {PARAM_NAMES.at(KEY_INDEX),     m_auth.key},
        {PARAM_NAMES.at(ID_INDEX),      m_video_details.id}
      }
    );

    json live_info = json::parse(r.text);

    if (!live_info.is_null() && live_info.is_object()) {
      auto items = live_info["items"];
      if (!items.is_null() && items.is_array() && items.size() > 0) {
        m_video_details.chat_id = items[0]["liveStreamingDetails"]["activeLiveChatId"].dump();
        SanitizeJSON(m_video_details.chat_id);
        if (!m_video_details.chat_id.empty()) {
          m_chats.insert({m_video_details.chat_id, std::vector<LiveMessage>{}});
          log("Added chat details for " + m_video_details.chat_id);
          return true;
        }
      }
    }
    return false;
  }

  /**
   * FetchChatMessages
   *
   * @returns [out] {std::string}
   */
  std::string YouTubeDataAPI::FetchChatMessages() {
  using namespace constants;
    if (m_auth.access_token.empty() || m_video_details.chat_id.empty()) return "";

    log("Fetching chat messages for " + m_video_details.chat_id);

    cpr::Response r = cpr::Get(
      cpr::Url{URL_VALUES.at(LIVE_CHAT_URL_INDEX)},
      cpr::Header{
        {HEADER_NAMES.at(ACCEPT_HEADER_INDEX), HEADER_VALUES.at(APP_JSON_INDEX)},
        {HEADER_NAMES.at(AUTH_HEADER_INDEX),   GetBearerAuth()}
      },
      cpr::Parameters{
        {PARAM_NAMES.at(PART_INDEX),           PARAM_VALUES.at(SNIPPET_INDEX)},
        {PARAM_NAMES.at(KEY_INDEX),            m_auth.key},
        {PARAM_NAMES.at(LIVE_CHAT_ID_INDEX),   m_video_details.chat_id},
      }
    );

    json chat_info = json::parse(r.text);

    if (!chat_info.is_null() && chat_info.is_object()) {
      auto      items = chat_info["items"];


      if (!items.is_null() && items.is_array() && !items.empty()) {
        for (const auto& item : items) {

          try {
            auto details       = item.at("snippet").at("textMessageDetails");
            std::string text   = (details.contains("messageText")) ?
                                   details["messageText"] :
                                   "";

            std::string author = item["snippet"]["authorChannelId"];
            std::string time   = item["snippet"]["publishedAt"];

            if (!IsNewer(time.c_str())) { // Ignore duplicates
              continue; // FIX: This is a bug -> tie is always old -> timezone issues?
            }

            SanitizeJSON(text);
            SanitizeJSON(author);
            SanitizeJSON(time);

            m_chats.at(m_video_details.chat_id).push_back(
              LiveMessage{
                .timestamp = time,
                .author    = author,
                .text      = text
              }
            );
          } catch (const std::exception& e) {
            std::string error_message{"Exception was caught: "};
            error_message += e.what();
            log(error_message);
          }
        }

        if (!m_chats.at(m_video_details.chat_id).empty()) {
          m_last_fetch_timestamp = to_unixtime(m_chats.at(m_video_details.chat_id).back().timestamp.c_str());
        }
      }
    }

    return r.text;
  }

  bool YouTubeDataAPI::IsNewer(const char* datetime) {
    return std::difftime(to_unixtime(datetime), m_last_fetch_timestamp) > 0;
  }

  /**
   * Parsetokens
   *
   * @returns
   *
   */
  bool YouTubeDataAPI::ParseTokens() {
    if (HasChats()) {
      for (auto&& chat : m_chats.at(m_video_details.chat_id)) {
        std::string tokenized_text = conversation::TokenizeText(chat.text);

        if (!tokenized_text.empty()) {
          chat.tokens = conversation::SplitTokens(tokenized_text);
        }
      }
    return (!GetCurrentChat().at(0).tokens.empty());
  }
  return false;
}

  /**
   * GetChats
   *
   * @returns [out] {LiveChatMap}
   */
  LiveChatMap YouTubeDataAPI::GetChats() {
    return m_chats;
  }

  /**
   * GetCurrentChat
   *
   * @param   [in]  {bool}
   * @returns [out] {LiveMessages}
   */
  LiveMessages YouTubeDataAPI::GetCurrentChat(bool keep_messages) {
    return m_chats.at(m_video_details.chat_id);
  }

  /**
   * HasChats
   *
   * @returns [out] {bool}
   */
  bool YouTubeDataAPI::HasChats() {
    return !m_chats.empty() && !GetCurrentChat().empty();
  }

  /**
   * InsertMessages
   *
   * @param   [in]  {std::string}
   * @param   [in]  {LiveMessages}
   * @returns [out] {bool}
   */
  bool YouTubeDataAPI::InsertMessages(std::string id, LiveMessages&& messages) {
    if (m_chats.find(id) != m_chats.end()) {
      for (auto&& message : messages) m_chats.at(id).emplace_back(message);
      return true;
    }
    return false;
  }

  /**
   * ClearChat
   *
   * @param   [in]  {std::string}
   * @returns [out] {bool}
   */
  bool YouTubeDataAPI::ClearChat(std::string id) {
    id = (id.empty()) ? m_video_details.chat_id : id;

    if (m_chats.find(id) != m_chats.end()) {
      m_chats.at(id).clear();
      return true;
    }

    return false;
  }
  /**
   * FindMentions
   *
   * @returns [out] {LiveMessages}
   */
  LiveMessages YouTubeDataAPI::FindMentions(bool keep_messages) {
    using ChatPair = std::map<std::string, LiveMessages>;
    const std::string bot_name = GetUsername();

    LiveMessages matches{};

    for (const Chat& m : GetChats()) {
      std::string            chat_name = m.first;
      LiveMessages           messages  = m.second;
      LiveMessages::iterator it        = messages.begin();

      for (; it != messages.end(); ) {
        LiveMessage message = *it;

        if (message.text.find(bot_name) != std::string::npos) {
          matches.push_back(message);

          (keep_messages) ?
            it++ :
            it = messages.erase(it);
        } else {
          it++;
        }
      }
    }
    return matches;
  }

  /**
   * FindChat
   *
   * @returns [out] {bool}
   */
  bool YouTubeDataAPI::FindChat() {
    if (m_auth.access_token.empty()) {
      if (GetAuth().access_token.empty()) {
        return false;
      }
    }

    if (FetchLiveVideoID().empty()) {
      return false;
    }

    if (!FetchLiveDetails()) {
      return false;
    }

    FetchChatMessages();

    return true;
  }

  /**
   * PostMessage
   *
   * @param
   * @returns
   */
  bool YouTubeDataAPI::PostMessage(std::string message) {
    using namespace constants;

    if (m_video_details.chat_id.empty()) {
      log("No chat to post to");
      return false;
    }

    log("Posting " + message);

    json payload{};
    payload["snippet"]["liveChatId"]                        = m_video_details.chat_id;
    payload["snippet"]["live_chat_id"]                      = m_video_details.chat_id;
    payload["snippet"]["textMessageDetails"]["messageText"] = message;
    payload["snippet"]["type"]                              = "textMessageEvent";

    cpr::Response r = cpr::Post(
      cpr::Url{URL_VALUES.at(LIVE_CHAT_URL_INDEX)},
      cpr::Header{
        {HEADER_NAMES.at(ACCEPT_HEADER_INDEX), HEADER_VALUES.at(APP_JSON_INDEX)},
        {HEADER_NAMES.at(AUTH_HEADER_INDEX),   GetBearerAuth()},
        {HEADER_NAMES.at(CONTENT_TYPE_INDEX),  HEADER_VALUES.at(APP_JSON_INDEX)}
      },
      cpr::Parameters{
        {PARAM_NAMES.at(PART_INDEX),           PARAM_VALUES.at(SNIPPET_INDEX)},
        {PARAM_NAMES.at(KEY_INDEX),            m_auth.key},
        {PARAM_NAMES.at(LIVE_CHAT_ID_INDEX),   m_video_details.chat_id},
      },
      cpr::Body{payload.dump()}
    );

    return r.status_code < 400;
  }

/**
 * GreetOnEntry
 *
 * @returns
 */
bool YouTubeDataAPI::GreetOnEntry() {
  return m_greet_on_entry;
}

/**
 * TestMode
 *
 * @returns [out] {bool}
 */
bool YouTubeDataAPI::TestMode() {
  return m_test_mode;
}

/**
 * RecordInteraction
 *
 * @param
 * @param
 */
void YouTubeDataAPI::RecordInteraction(std::string id, Interaction interaction, std::string value) {
  if (m_interactions.find(id) == m_interactions.end()) {
    std::pair<std::string, bool> interaction_pair{value, true};
    m_interactions.insert({id, UserInteraction{.id = id}});
    if (interaction == Interaction::greeting) {
      m_persons.insert(interaction_pair);
    }
    else
    if (interaction == Interaction::location_ask) {
      m_locations.insert(interaction_pair);
    }
    else
    if (interaction == Interaction::probing) {
      m_orgs.insert(interaction_pair);
    }
  }

  UserInteraction& user_interaction = m_interactions.at(id);

  if (interaction == Interaction::greeting) {
    user_interaction.greeted = true;
  }
  else
  if (interaction == Interaction::promotion) {
    user_interaction.promoted = true;
  }
  else
  if (interaction == Interaction::probing) {
    user_interaction.probed = true;
  }
  else
  if (interaction == Interaction::location_ask) {
    user_interaction.location = true;
  }
}

/**
 * HasInteracted
 */
bool YouTubeDataAPI::HasInteracted(std::string id, Interaction interaction) {
  if (m_interactions.find(id) == m_interactions.end()) {
    return false;
  }

  UserInteraction& user_interaction = m_interactions.at(id);

  bool has_interacted{false};

  if (interaction == Interaction::greeting) {
    has_interacted = user_interaction.greeted;
  }
  else
  if (interaction == Interaction::promotion) {
    has_interacted = user_interaction.promoted;
  }
  else
  if (interaction == Interaction::probing) {
    has_interacted = user_interaction.probed;
  }
  else
  if (interaction == Interaction::location_ask) {
    has_interacted = user_interaction.location;
  }

  return has_interacted;
}

/**
 *
 */
bool YouTubeDataAPI::HasDiscussed(std::string value, Interaction type) {
  std::map<std::string, bool>::iterator it{};
  if (type == Interaction::greeting) {
    it = m_persons.find(value);
  }
  else
  if (type == Interaction::promotion) {
    // TODO: ?
  }
  else
  if (type == Interaction::probing) {
    it = m_orgs.find(value);
  }
  else
  if (type == Interaction::location_ask) {
    it = m_locations.find(value);
  }
  return (it->second == true);
}

} // namespace kbot
