#ifndef __UTIL_HPP__
#define __UTIL_HPP__

/**
 * Poor man's log
 */
template<typename T>
void log(T s) {
  std::cout << s << std::endl;
}

/**
 * SanitizeJSON
 *
 * Helper function to remove escaped double quotes from a string
 *
 * @param [in] {std::string&} A reference to a string object
 */
void SanitizeJSON(std::string& s) {
  s.erase(
    std::remove(s.begin(), s.end(),'\"'),
    s.end()
  );
}

#endif // __UTIL_HPP__
