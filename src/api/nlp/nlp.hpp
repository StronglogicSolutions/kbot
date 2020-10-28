#ifndef __NLP_HPP
#define __NLP_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include "types.hpp"


namespace conversation {
extern const std::string TOKENIZER_PATH;

const  std::string        get_executable_cwd();
       std::string        TokenizeText(std::string s);
       std::vector<Token> SplitTokens(std::string s);
       Token              ParseToken(std::string s);
       TokenType          GetType(std::string type);

class NLP {
 public:
  NLP() {}
  void           Insert(Message&& node, std::string name, std::string subject);
  Map            GetConversations() { return m_m; }
  const Message* GetConversation(std::string name, std::string subject) { return m_m.at(Context{name, subject}); }
  std::string    toString();

 private:
  Map            m_m; // Pointer map
  MessageObjects m_q; // Queue of objects
};

} // namespace Conversation
#endif // __NLP_HPP