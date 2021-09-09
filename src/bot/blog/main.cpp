#include "blog.hpp"

int main(int argc, char** argv)
{
  kbot::BlogBot bot{};
  bot.BlogTest(std::string(argv[1]), argv[2]);

  std::cout << "Blog bot!" << std::endl;

  return 0;
}