#include "Tap.hpp"

namespace paisa {

Tap::Tap(float *bL, float *bR, size_t size, size_t *wIdx) {
  delay =
      std::unique_ptr<DelayProcessor>(new DelayProcessor(bL, bR, size, wIdx));
  filter = std::unique_ptr<FilterProcessor>(new FilterProcessor());
  amppan = std::unique_ptr<AmpPanProcessor>(new AmpPanProcessor());
  fx1 = std::unique_ptr<FX1Processor>(new FX1Processor());
  fx2 = std::unique_ptr<FX2Processor>(new FX2Processor());

  processors.push_back(delay.get());
  processors.push_back(amppan.get());
  processors.push_back(filter.get());
  processors.push_back(fx1.get());
  processors.push_back(fx2.get());
}

void Tap::setParam(int mode, float p1, float p2) {
  if (mode >= 0 && mode < (int)processors.size()) {
    processors[mode]->setParams(p1, p2);
  }
}

void Tap::setOffset(int mode, float o1, float o2) {
  if (mode >= 0 && mode < (int)processors.size()) {
    processors[mode]->setOffsets(o1, o2);
  }
}

void Tap::process(float &left, float &right, float sampleRate) {
  // Current sample starts as 0 (we read from delay buffer first)
  left = 0.0f;
  right = 0.0f;

  // Process in order: Delay (reads from buffer) -> AmpPan -> Filter -> FX1 ->
  // FX2
  for (auto *p : processors) {
    p->process(left, right, sampleRate);
  }
}

} // namespace paisa
