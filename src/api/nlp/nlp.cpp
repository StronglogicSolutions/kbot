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
  m_q.emplace_back(std::move(node)); // Object lives in queue
  Message* node_ref = &m_q.back();
  const Map::const_iterator it = m_m.find(Context{name, subject});
  // Insert new head
  if ( it == m_m.end()) {
    node_ref->next = nullptr;
    m_m.insert({
      Context{name, subject},
      node_ref
    });
  } else {
    const Message* previous_head = &(*it->second);
    node_ref->next               = previous_head;

    m_m.erase(it);
    m_m.insert({{name, subject}, node_ref});
  }
}

/**
 *
 */
std::string NLP::toString() {
  std::string node_string{};

  for (const auto& c : m_m) {
    const Message* node = c.second;

    node_string += "Interlocutor: " + c.first.user + "\nSubject: " + c.first.subject + "\n";

    while ( node != nullptr) {
        node_string += node->text + "\n";
        node = node->next;
    }
    node_string += "\n";
  }
  return node_string;
}
} // namespace conversation