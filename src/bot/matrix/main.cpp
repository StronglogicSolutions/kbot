#include "matrix.hpp"

int main(int argc, char** argv)
{
  // kbot

  // std::cout << "Matrix bot!" << std::endl;

  katrix::KatrixBot bot{"matrix.org", "logicp", "59rszGbn6vTpr7rs"};
  bot.login();

  for (int i = 0; i < 5; i++)
  {
    auto logged_in = bot.logged_in();
    kbot::log("Bot login status ", std::to_string(logged_in).c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  return 0;
}