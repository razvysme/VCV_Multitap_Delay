#pragma once
#include <rack.hpp>

namespace paisa {

class Processor {
public:
  virtual ~Processor() = default;
  virtual void process(float &left, float &right, float sampleRate) = 0;
  virtual void setParams(float p1, float p2) = 0;
  virtual void setOffsets(float o1, float o2) = 0;
};

} // namespace paisa
