#pragma once
#include <rack.hpp>

namespace paisa {

/**
 * Efficient State Variable Filter (Trapezoidal / Simper)
 */
struct SVF {
  float s1 = 0.f, s2 = 0.f;
  float g = 0.f, k = 0.f;
  float a1 = 0.f, a2 = 0.f, a3 = 0.f;

  void reset() { s1 = s2 = 0.f; }

  void setParams(float freq, float res) {
    // freq: normalized frequency [0, 0.5]
    // res: resonance [0, 1]
    g = std::tan(M_PI * std::min(freq, 0.49f));
    k = 2.0f - 2.0f * res;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
  }

  void process(float x, float &low, float &high, float &band) {
    float v3 = x - s2;
    float v1 = a1 * s1 + a2 * v3;
    float v2 = s2 + a2 * s1 + a3 * v3;
    s1 = 2.0f * v1 - s1;
    s2 = 2.0f * v2 - s2;
    low = v2;
    band = v1;
    high = x - k * v1 - v2;
  }
};

/**
 * Base-Width Filter implementation using two SVFs in a parallel configuration.
 * Base: Controls the High-Pass cutoff frequency.
 * Width: Controls the range above Base for the Low-Pass cutoff frequency.
 */
struct BaseWidthFilter {
  SVF hp_filter;
  SVF lp_filter;

  void reset() {
    hp_filter.reset();
    lp_filter.reset();
  }

  void setParams(float sampleRate, float baseFreq, float widthRange) {
    float f1 = baseFreq / sampleRate;
    // Map width (0-100) to an octave range (e.g., 0 to 10 octaves)
    float octaves = (widthRange / 100.0f) * 10.0f;
    float f2 = (baseFreq * std::pow(2.0f, octaves)) / sampleRate;

    hp_filter.setParams(f1, 0.0f); // HP component (base)
    lp_filter.setParams(f2, 0.0f); // LP component (width extension)
  }

  float process(float x) {
    float l1, h1, b1;
    float l2, h2, b2;
    hp_filter.process(x, l1, h1, b1);
    lp_filter.process(x, l2, h2, b2);

    // The base-width effect is achieved by removing the signal
    // below the HP cutoff (l1) and above the LP cutoff (h2).
    // Output = Input - (Lower-Stopped-Band + Upper-Stopped-Band)
    return x - (l1 + h2);
  }
};

} // namespace paisa
