#pragma once
#include "AmpPanProcessor.hpp"
#include "DelayProcessor.hpp"
#include "FXProcessors.hpp"
#include "FilterProcessor.hpp"
#include <memory>
#include <vector>

namespace paisa {

class Tap {
  std::unique_ptr<DelayProcessor> delay;
  std::unique_ptr<FilterProcessor> filter;
  std::unique_ptr<AmpPanProcessor> amppan;
  std::unique_ptr<FX1Processor> fx1;
  std::unique_ptr<FX2Processor> fx2;

public:
  Tap(size_t bufferSize);

  void setParam(int mode, float p1, float p2);
  void setFX2Params(float p1, float p2, float p3);
  void process(float inL, float inR, float &outL, float &outR,
               float sampleRate);
};

} // namespace paisa
