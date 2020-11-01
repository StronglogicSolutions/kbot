#include "nlp.hpp"

namespace conversation {
const std::string TOKENIZER_PATH{"third_party/MITIE/tools/ner_stream/ner_stream third_party/MITIE/MITIE-models/english/ner_model.dat > tokenized_message.txt > tokenized_message.txt"};

const std::string get_executable_cwd() {
  char* path = realpath("/proc/self/exe", NULL);
  char* name = basename(path);
  return std::string{path, path + strlen(path) - strlen(name)};
}

std::string TokenizeText(std::string s) {
  std::string execution_line{};
  execution_line.reserve(50);

  execution_line += "echo \"" + s + "\" | " + TOKENIZER_PATH;

  std::system(execution_line.c_str());
    return std::string{
      static_cast<std::stringstream const&>(
        std::stringstream() << std::ifstream("tokenized_message.txt").rdbuf())
        .str()};
}

/**
   * GetType
   *
   * @param
   * @returns
   *
   */
  TokenType GetType(std::string type) {
    if (type.compare("LOCATION") == 0) {
      return TokenType::location;
    }
    else
    if (type.compare("PERSON") == 0) {
      return TokenType::person;
    }
    else
    if (type.compare("ORGANIZATION") == 0) {
      return TokenType::organization;
    }
    return TokenType::unknown;
  }

  /**
   * ParseToken
   *
   * @param
   * @returns
   *
   */
  Token ParseToken(std::string s) {
    auto delim = s.find(' ');
    return Token{
      .type  = GetType(s.substr(0, delim)),
      .value = s.substr(delim + 1)
    };
  }

  /**
   * SplitTokens
   *
   * @param
   * @returns
   *
   */
  std::vector<Token> SplitTokens(std::string s) {
    std::vector<Token> tokens{};
    auto               delim_index = s.find_first_of('[');

    while (delim_index != std::string::npos) {
      auto token_start     = s.substr(delim_index);
      auto delim_end_index = (token_start.find_first_of(']') - 1);
      auto token_value     = token_start.substr(1, delim_end_index);

      tokens.push_back(ParseToken(token_value));

      if (token_start.size() >= (token_value.size() + 3)) {
        s           = token_start.substr(token_value.size() + 3);
        delim_index = s.find_first_of('[');
      } else {
        break;
      }
    }

    return tokens;
  }



/**
 *
 */
void NLP::Insert(Message&& node, std::string name, std::string subject) {
  m_q.emplace_back(std::move(node));                         // Object lives in queue
  Message* node_ref = &m_q.back();
  const Map::const_iterator it = m_m.find(name);

  if ( it == m_m.end()) {                                    // New
    m_s.emplace_back(std::move(SubjectiveContext{subject}));
    SubjectiveContext* s_ctx_ref = &m_s.back();

    node_ref->subjective = s_ctx_ref;
    node_ref->next = nullptr;

    m_m.insert({
      name,
      node_ref
    });
  } else {                                                    // Append
    Message* previous_head       = &(*it->second);
    node_ref->next               = previous_head;
    node_ref->subjective         = previous_head->subjective;
    node_ref->subjective->Insert(subject);

    m_m.erase(it);
    m_m.insert({name, node_ref});
  }
}

/**
 * Reply
 */
void NLP::Reply(Message* node, std::string reply, std::string name) {
  Message reply_node{
    .text = reply,
    .received = false,
    .next = node->next,
    .subjective = node->subjective
  };

  m_q.emplace_back(std::move(reply_node));
  Message* reply_node_ref = &m_q.back();

  node->next = reply_node_ref;
}

/**
 * toString
 */
std::string NLP::toString() {
  std::string node_string{};

  for (const auto& c : m_m) {
    const Message* node = c.second;

    node_string +=
"┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n│                                                                                                                    │\n\
│░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│\n\
│░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░CONVERSATION░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│\n\
│░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│\n│";

    node_string += "\n│Interlocutor: " + c.first +
                   "\n│Subjective: " + c.second->subjective->toString() +
                   "\n│Objective: "  + c.second->objective->toString() +
                   "\n│Nodes:\n";

    uint8_t n_idx{1};
    while ( node != nullptr) {
        node_string += "│ " + std::to_string(n_idx) + ": ";
        for (int i = 0; i <= n_idx; i++)
          node_string += "  ";
        node_string += (n_idx > 1) ? "↳" : "";
        node_string += node->text + "\n";
        node = node->next;
        n_idx++;
    }
    node_string +=
"│                                                                                                                    │\n│                                                                                                                    │\n└────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘\n\n";
  }
  return node_string;
}
} // namespace conversation