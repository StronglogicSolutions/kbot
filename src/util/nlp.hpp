#ifndef __NLP_HPP
#define __NLP_HPP

#include <string>
#include <cstring>
#include <sstream>

const std::string TOKENIZER_PATH{""};

const std::string get_executable_cwd() {
  char* path = realpath("/proc/self/exe", NULL);
  char* name = basename(path);
  return std::string{path, path + strlen(path) - strlen(name)};
}

std::string TokenizeText(std::string s) {
  std::string execution_line{};
  execution_line.reserve(50);

  execution_line += "echo \"" + s + "\" | third_party/MITIE/tools/ner_stream/ner_stream third_party/MITIE/MITIE-models/english/ner_model.dat > tokenized_message.txt > tokenized_message.txt";

  std::system(execution_line.c_str());
    return std::string{
        static_cast<std::stringstream const&>(
            std::stringstream() << std::ifstream("tokenized_message.txt").rdbuf())
            .str()};
}

#endif // __NLP_HPP