#pragma once
#include "Filter.hpp"
#include "Processor.hpp"
#include <algorithm>
#include <cmath>

namespace paisa {

class FilterProcessor : public Processor {
  BaseWidthFilter filterL;
  BaseWidthFilter filterR;
  float normalizedFreq = 0.5f;
  float normalizedWidth = 0.5f;
  float lastSampleRate = -1.0f;
  float lastFreq = -1.0f;
  float lastWidth = -1.0f;
  bool dirty = true;

public:
  void setParams(float p1, float p2) override {
    if (p1 != normalizedFreq || p2 != normalizedWidth) {
      normalizedFreq = p1;
      normalizedWidth = p2;
      dirty = true;
    }
  }

  void process(float &left, float &right, float sampleRate) override {
    // Only update filter coefficients if parameters or sample rate changed
    if (dirty || sampleRate != lastSampleRate) {
      // Clip the implementation for algorithm safety
      float k1 = std::max(0.0f, std::min(1.0f, normalizedFreq));
      float k2 = std::max(0.0f, std::min(1.0f, normalizedWidth));

      // Logarithmic curve for frequency (Octaves)
      float f =
          std::exp(std::log(20.f) + k1 * (std::log(20000.f) - std::log(20.f)));

      // Octave-based width offset (Consistent window feel)
      // k2 = 0 -> HP and LP coincide (Filter bypassed/spike)
      // k2 = 1 -> LP is 10 octaves above HP
      float w = k2 * 100.f;

      if (f != lastFreq || w != lastWidth || sampleRate != lastSampleRate) {
        filterL.setParams(sampleRate, f, w);
        filterR.setParams(sampleRate, f, w);
        lastFreq = f;
        lastWidth = w;
        lastSampleRate = sampleRate;
      }
      dirty = false;
    }

    left = filterL.process(left);
    right = filterR.process(right);
  }
};

} // namespace paisa
