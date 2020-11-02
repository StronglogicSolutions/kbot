#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <ctime>
#include <iomanip>

/**
 * Poor man's log
 */
template<typename T>
inline void log(T s) {
  std::cout << s << std::endl;
}

/**
 * SanitizeJSON
 *
 * Helper function to remove escaped double quotes from a string
 *
 * @param [in] {std::string&} A reference to a string object
 */
inline void SanitizeJSON(std::string& s) {
  s.erase(
    std::remove(s.begin(), s.end(),'\"'),
    s.end()
  );
}


inline const std::time_t to_unixtime(const char* datetime) {
  std::tm            t{};
  std::istringstream ss{datetime};

  ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");

  return mktime(&t);
}

#endif // __UTIL_HPP__
