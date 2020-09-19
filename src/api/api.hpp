#ifndef __API_HPP__
#define __API_HPP__

#include "interfaces/interfaces.hpp"

class DefaultAPI : public API {
public:

virtual std::string GetType() override {
  return std::string{"Default API"};
}

};
#endif // __API_HPP__