#pragma once
#include "Panner.hpp"
#include "Processor.hpp"
#include <algorithm>
#include <cmath>

namespace paisa {

class AmpPanProcessor : public Processor {
  float normalizedAmp = 0.5f;
  float normalizedPan = 0.5f;
  Panner panner;

  // Cached values
  float gain = 1.0f;
  float panValue = 0.0f;
  bool dirty = true;

public:
  void setParams(float p1, float p2) override {
    if (p1 != normalizedAmp || p2 != normalizedPan) {
      normalizedAmp = p1;
      normalizedPan = p2;
      dirty = true;
    }
  }

  void process(float &left, float &right, float sampleRate) override {
    if (dirty) {
      // Clip the implementation
      float k1 = std::max(0.0f, std::min(1.0f, normalizedAmp));
      float k2 = std::max(0.0f, std::min(1.0f, normalizedPan));

      // Logarithmic curve for amplitude (linear in dB)
      // Consistent with how volume is perceived
      float gainDb = k1 * 78.f - 72.f; // -72dB to +6dB
      gain = std::pow(10.f, gainDb / 20.f);

      // Linear for panning (the Panner internally handles constant power law)
      panValue = k2 * 2.f - 1.f; // Map [0, 1] to [-1, 1]

      dirty = false;
    }

    left *= gain;
    right *= gain;

    panner.process(left, right, panValue);
  }
};

} // namespace paisa
