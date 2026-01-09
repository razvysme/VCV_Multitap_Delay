#pragma once
#include "Processor.hpp"

namespace paisa {

class FX1Processor : public Processor {
public:
  void setParams(float p1, float p2) override {}
  void setOffsets(float o1, float o2) override {}
  void process(float &left, float &right, float sampleRate) override {
    // Pass through
  }
};

class FX2Processor : public Processor {
public:
  void setParams(float p1, float p2) override {}
  void setOffsets(float o1, float o2) override {}
  void process(float &left, float &right, float sampleRate) override {
    // Pass through
  }
};

} // namespace paisa
