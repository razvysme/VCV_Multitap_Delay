#pragma once
#include "FrequencyShifter.hpp"
#include "Processor.hpp"

namespace paisa {

class FX1Processor : public Processor {
private:
  FrequencyShifter shifter;

public:
  void setParams(float p1, float p2) override { shifter.setParams(p1, p2); }
  void process(float &left, float &right, float sampleRate) override {
    shifter.process(left, right, sampleRate);
  }
};

class FX2Processor : public Processor {
public:
  void setParams(float p1, float p2) override {}
  void process(float &left, float &right, float sampleRate) override {
    // Pass through
  }
};

} // namespace paisa
