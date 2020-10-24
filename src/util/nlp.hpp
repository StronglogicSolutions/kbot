#ifndef __NLP_HPP
#define __NLP_HPP

#include <string>
#include <cstring>
#include <sstream>
#include <vector>
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
  std::string text;
  bool        received;
  Message*    next;
};

class MyPair : public std::pair<Context, Message*> {
  typedef std::pair<Context, Message*> parent_type;
  public:
  using parent_type::parent_type;

  MyPair() : parent_type{Context{"", "" }, nullptr} {}
};


using Map            = std::map<Context, Message*>;
using MessageObjects = std::vector<Message>;

class S {

public:
S() : m_m{{Context("Initial", "place"), nullptr}} {}
void Insert(Message&& node, std::string name, std::string subject) {
  const Map::const_iterator it = m_m.find(Context{name, subject});
  // Insert new head
  if ( it == m_m.end()) {
    m_m.insert({Context{name, subject}, &node});
  } else {
    Message* previous_head = it->second;

    node.next = previous_head;
    m_m.at({name, subject}) = &node;
  }
  m_v.emplace_back(std::move(node)); // Object lives in vector
}

void print() {
  for (const auto& c : m_m) {
    Message* node = c.second;

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

Message* GetConversation(std::string name, std::string subject) {
  return m_m.at(Context{name, subject});
}

private:
  Map            m_m; // pointer map
  MessageObjects m_v; // vector of objects
};

} // namespace Conversation
#endif // __NLP_HPP