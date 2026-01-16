#pragma once
#include "FrequencyShifter.hpp"
#include "Phaser.hpp"
#include "Processor.hpp"

namespace paisa {

class FX1Processor : public Processor {
private:
  FrequencyShifter shifter;

public:
  void setParams(float p1, float p2) override { shifter.setParams(p1, p2); }
  void setParams(float p1, float p2, float p3) override {
    shifter.setParams(p1, p2);
  }
  void process(float &left, float &right, float sampleRate) override {
    shifter.process(left, right, sampleRate);
  }
};

class FX2Processor : public Processor {
private:
  Phaser phaser;

public:
  void setParams(float p1, float p2) override { phaser.setParams(p1, p2); }
  void setParams(float p1, float p2, float p3) override {
    phaser.setParams(p1, p2, p3);
  }
  void process(float &left, float &right, float sampleRate) override {
    phaser.process(left, right, sampleRate);
  }
};

} // namespace paisa
