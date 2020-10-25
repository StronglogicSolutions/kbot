#ifndef __NLP_HPP
#define __NLP_HPP

#include <string>
#include <cstring>
#include <sstream>
#include <deque>
#include <map>

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

namespace Conversation {

struct Context {
std::string user;
std::string subject;
Context(std::string user_name, std::string subject_name) : user(user_name), subject(subject_name) {}

bool operator <(const Context &rhs) const {
  return (
    user < rhs.user && subject < rhs.subject
  );
}

};

struct Message {
  std::string       text;
  bool              received;
  const Message*    next;
};

using Map            = std::map<Context, const Message*>;
using MessageObjects = std::deque<Message>;

class S {

public:
S() {}
void Insert(Message&& node, std::string name, std::string subject) {
  m_v.emplace_back(std::move(node)); // Object lives in queue
  Message* node_ref = &m_v.back();
  const Map::const_iterator it = m_m.find(Context{name, subject});
  // Insert new head
  if ( it == m_m.end()) {
    node_ref->next = nullptr;
    m_m.insert({Context{name, subject}, node_ref});
  } else {
    const Message* previous_head = &(*it->second);
    node_ref->next               = previous_head;

    m_m.erase(it);
    m_m.insert({{name, subject}, node_ref});
  }
}

void print() {
  for (const auto& c : m_m) {
    const Message* node = c.second;

    std::cout << "Interlocutor: " << c.first.user << "\nSubject: " << c.first.subject << std::endl;
    while ( node != nullptr) {
        std::cout << node->text;
        node = node->next;
    }
    std::cout << std::endl;
  }
}

Map GetConversations() {
  return m_m;
}

const Message* GetConversation(std::string name, std::string subject) {
  return m_m.at(Context{name, subject});
}

private:
  Map            m_m; // Pointer map
  MessageObjects m_v; // Queue of objects
};

} // namespace Conversation
#endif // __NLP_HPP