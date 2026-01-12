#pragma once
#include <algorithm>
#include <cmath>
#include <rack.hpp>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace paisa {

// 63-tap FIR Hilbert Transformer for producing accurate quadrature
class FIRHilbert {
private:
  static constexpr int TAPS = 63;
  static constexpr int M = 31; // (TAPS - 1) / 2
  float kernel[TAPS];
  float buffer[TAPS];
  int writeIdx = 0;

public:
  FIRHilbert() {
    std::fill(buffer, buffer + TAPS, 0.0f);
    for (int n = 0; n < TAPS; n++) {
      int k = n - M;
      if (k == 0 || (k % 2 == 0)) {
        kernel[n] = 0.0f;
      } else {
        // Ideal Hilbert: h[k] = 2 / (pi * k) for odd k
        // Apply Blackman window to smooth response
        float window = 0.42f - 0.5f * std::cos(2.0f * M_PI * n / (TAPS - 1)) +
                       0.08f * std::cos(4.0f * M_PI * n / (TAPS - 1));
        kernel[n] = window * (2.0f / (M_PI * (float)k));
      }
    }
  }

  float process(float x) {
    buffer[writeIdx] = x;
    float sum = 0.0f;
    for (int i = 0; i < TAPS; i++) {
      int readIdx = writeIdx - i;
      if (readIdx < 0)
        readIdx += TAPS;
      sum += buffer[readIdx] * kernel[i];
    }
    writeIdx = (writeIdx + 1) % TAPS;
    return sum;
  }
};

// Simple delay line to match FIR group delay (31 samples)
class MatchingDelay {
private:
  static constexpr int DELAY = 31;
  float buffer[DELAY + 1];
  int writeIdx = 0;

public:
  MatchingDelay() { std::fill(buffer, buffer + DELAY + 1, 0.0f); }
  float process(float x) {
    float out = buffer[writeIdx];
    buffer[writeIdx] = x;
    writeIdx = (writeIdx + 1) % (DELAY + 1);
    return out;
  }
};

class BiquadHPF {
private:
  float z1 = 0.0f, z2 = 0.0f;
  float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;

public:
  void setParams(float freq, float sampleRate) {
    if (sampleRate < 1.0f)
      return;
    float w0 = 2.0f * M_PI * freq / sampleRate;
    float cosw0 = std::cos(w0);
    float alpha = std::sin(w0) / (2.0f * 0.707f);
    float a0 = 1.0f + alpha;
    b0 = (1.0f + cosw0) / 2.0f / a0;
    b1 = -(1.0f + cosw0) / a0;
    b2 = (1.0f + cosw0) / 2.0f / a0;
    a1 = -2.0f * cosw0 / a0;
    a2 = (1.0f - alpha) / a0;
  }

  float process(float x) {
    float out = b0 * x + z1;
    z1 = b1 * x - a1 * out + z2;
    z2 = b2 * x - a2 * out;
    if (!std::isfinite(z1))
      z1 = 0.0f;
    if (!std::isfinite(z2))
      z2 = 0.0f;
    return out;
  }
};

class OnePoleLPF {
private:
  float y_z1 = 0.0f;

public:
  float process(float x, float k) {
    y_z1 = y_z1 + k * (x - y_z1);
    if (!std::isfinite(y_z1))
      y_z1 = 0.0f;
    return y_z1;
  }
};

class FrequencyShifter {
private:
  FIRHilbert hilbertL, hilbertR;
  MatchingDelay delayL, delayR;
  BiquadHPF hpfL, hpfR;
  OnePoleLPF postL, postR;

  // Independent oscillators for coherence / spec compliance
  float osc_cL = 1.0f, osc_sL = 0.0f;
  float osc_cR = 1.0f, osc_sR = 0.0f;
  int renormalizeCounter = 0;

  float targetShift = 0.0f;
  float targetWet = 0.0f;
  float targetSignedShift = 0.0f;

  float currentWet = 0.0f;
  float currentSignedShift = 0.0f;

  float lastSampleRate = 0.0f;
  int blockCounter = 0;
  float cosD = 1.0f;
  float sinD = 0.0f;

public:
  void setParams(float k1, float k2) {
    const float F_min = 50.0f;
    const float F_max = 5000.0f;
    targetShift = F_min + (F_max - F_min) * std::pow(k1, 2.0f);

    float wetRaw = k2 * 2.0f - 1.0f;
    float detent = 0.04f;
    if (std::abs(wetRaw) < detent) {
      targetWet = 0.0f;
      targetSignedShift = 0.0f;
    } else {
      targetWet = std::abs(wetRaw);
      targetSignedShift = (wetRaw > 0.0f ? 1.0f : -1.0f) * targetShift;
    }
  }

  void process(float &left, float &right, float sampleRate) {
    if (sampleRate < 1.0f)
      return;

    if (std::abs(sampleRate - lastSampleRate) > 1.0f) {
      hpfL.setParams(40.0f, sampleRate);
      hpfR.setParams(40.0f, sampleRate);
      lastSampleRate = sampleRate;
    }

    const float tauWet = 0.015f;
    const float tauShift = 0.030f;
    float lambdaWet = 1.0f - std::exp(-1.0f / (tauWet * sampleRate));
    float lambdaShift = 1.0f - std::exp(-1.0f / (tauShift * sampleRate));

    currentWet += (targetWet - currentWet) * lambdaWet;
    currentSignedShift +=
        (targetSignedShift - currentSignedShift) * lambdaShift;

    if (!std::isfinite(currentWet))
      currentWet = 0.0f;
    if (!std::isfinite(currentSignedShift))
      currentSignedShift = 0.0f;

    // Block-rate trig update (shared delta, independent update inside channel)
    if (--blockCounter <= 0) {
      blockCounter = 16;
      float delta = 2.0f * M_PI * currentSignedShift / sampleRate;
      cosD = std::cos(delta);
      sinD = std::sin(delta);
    }

    float absShift = std::abs(currentSignedShift);
    float kLPF = 1.0f - (absShift / 5000.0f) * 0.8f;
    kLPF = rack::math::clamp(kLPF, 0.1f, 1.0f);

    float *ch[2] = {&left, &right};
    float *osc_c[2] = {&osc_cL, &osc_cR};
    float *osc_s[2] = {&osc_sL, &osc_sR};

    for (int i = 0; i < 2; ++i) {
      float in = (i == 0) ? hpfL.process(*ch[i]) : hpfR.process(*ch[i]);

      // Recursive Rotation per channel
      float c = *osc_c[i];
      float s = *osc_s[i];
      float next_c = c * cosD - s * sinD;
      float next_s = s * cosD + c * sinD;
      *osc_c[i] = next_c;
      *osc_s[i] = next_s;

      // Analytic Signal Generation via FIR Hilbert
      float x_real = (i == 0) ? delayL.process(in) : delayR.process(in);
      float x_imag = (i == 0) ? hilbertL.process(in) : hilbertR.process(in);

      // SSB Recombination (USB for positive shift):
      // y = x_real * cos - x_imag * sin
      float shifted = x_real * next_c - x_imag * next_s;

      float theta = (M_PI * 0.5f) * currentWet;
      float gDry = std::cos(theta);
      float gWet = std::sin(theta);
      float mixed = gDry * (*ch[i]) + gWet * shifted;

      float output =
          (i == 0) ? postL.process(mixed, kLPF) : postR.process(mixed, kLPF);

      float limit = 0.95f;
      if (output > limit)
        output = limit +
                 (1.0f - limit) * std::tanh((output - limit) / (1.0f - limit));
      else if (output < -limit)
        output = -limit -
                 (1.0f - limit) * std::tanh((-output - limit) / (1.0f - limit));

      if (!std::isfinite(output))
        output = 0.0f;
      *ch[i] = output;
    }

    if (++renormalizeCounter >= 512) {
      renormalizeCounter = 0;
      float rL = 1.0f / std::sqrt(osc_cL * osc_cL + osc_sL * osc_sL);
      osc_cL *= rL;
      osc_sL *= rL;
      float rR = 1.0f / std::sqrt(osc_cR * osc_cR + osc_sR * osc_sR);
      osc_cR *= rR;
      osc_sR *= rR;
    }
  }
};

} // namespace paisa
