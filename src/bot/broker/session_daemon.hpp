// #include <chrono>

// using time_point = std::chrono::time_point<std::chrono::system_clock>;
// using duration   = std::chrono::milliseconds;

// static const duration time_limit = std::chrono::milliseconds(60000);a

// class session_daemon {
// public:
//   bool reset()
//   {
//     const time_point tp = std::chrono::system_clock::now();
//     m_duration = (tp - m_tp);
//     return (m_duration > time_limit);
//   }

//   void begin()
//   {
//     m_tp = std::chrono::system_clock::now();
//   }

// private:
// time_point m_tp;
// duration   m_duration;

// };
