#ifndef __API_HPP__
#define __API_HPP__
#include <cpr/cpr.h>
#include "interfaces/interfaces.hpp"

class DefaultAPI : public API {
public:

virtual std::string GetType() override {
  return std::string{"Default API"};
}

};

class RequestAPI : public API {
  public:

  virtual std::string GetType() override {
    return std::string{"Request API"};
  }

  std::string Get() {
    cpr::Response r = cpr::Get(
      cpr::Url{"https://cointrx.com/prices/graph/json"}
    );

    return r.text;
  }
};

#endif // __API_HPP__
