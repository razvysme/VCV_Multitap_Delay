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

  std::vector<Processor *> processors;

public:
  Tap(float *bL, float *bR, size_t size, size_t *wIdx);

  void setParam(int mode, float p1, float p2);
  void setOffset(int mode, float o1, float o2);
  void process(float &left, float &right, float sampleRate);
};

} // namespace paisa
