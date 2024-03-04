#include "korean.test.hpp"

TEST(KoreanAPITest, InstantiateAPI) {
  EXPECT_NO_THROW(kiq::korean::KoreanAPI{});
}

TEST(KoreanAPITest, TranslateText) {
  kiq::korean::KoreanAPI api{};
  std::string test_string{"This string definitely mentions Korean"};
  EXPECT_EQ(api.MentionsKorean(test_string), true);
}
