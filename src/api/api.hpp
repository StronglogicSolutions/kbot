#ifndef __API_HPP__
#define __API_HPP__

#include "interfaces/interfaces.hpp"

class DefaultApi : public Api {
public:

virtual std::string GetType() override {
  return std::string{"Default API"};
}

};
#endif // __API_HPP__