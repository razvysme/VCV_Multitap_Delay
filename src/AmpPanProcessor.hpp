#pragma once
#include "Panner.hpp"
#include "Processor.hpp"
#include <algorithm>
#include <cmath>

namespace paisa {

class AmpPanProcessor : public Processor {
  float gainDb = 0.0f;
  float pan = 0.0f;
  float offsetGainDb = 0.0f;
  float offsetPan = 0.0f;
  Panner panner;

public:
  void setParams(float p1, float p2) override {
    gainDb = p1 * 78.f - 72.f;
    pan = p2 * 2.f - 1.f;
  }

  void setOffsets(float o1, float o2) override {
    offsetGainDb = o1 * 48.f;
    offsetPan = o2;
  }

  void process(float &left, float &right, float sampleRate) override {
    float totalDb = std::max(-100.f, std::min(24.f, gainDb + offsetGainDb));
    float totalPan = std::max(-1.f, std::min(1.f, pan + offsetPan));

    float gain = std::pow(10.f, totalDb / 20.f);

    // Apply gain first
    left *= gain;
    right *= gain;

    // Apply constant power panning
    panner.process(left, right, totalPan);
  }
};

} // namespace paisa
