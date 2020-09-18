#ifndef __INTERFACES_HPP__
#define __INTERFACES_HPP__

class Bot {
 public:
  virtual ~Bot() {}
  virtual void run() = 0;
};

#endif // __INTERFACES_HPP__