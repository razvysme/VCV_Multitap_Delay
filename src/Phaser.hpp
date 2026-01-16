#pragma once
#include <cmath>
#include <cstdlib>
#include <rack.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace paisa {

/**
 * Pink Noise Generator (Paul Kellet's economy method)
 */
class PinkNoise {
private:
  float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f,
        b6 = 0.0f;

public:
  float process() {
    float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    b0 = 0.99886f * b0 + white * 0.0555179f;
    b1 = 0.99332f * b1 + white * 0.0750759f;
    b2 = 0.96900f * b2 + white * 0.1538520f;
    b3 = 0.86650f * b3 + white * 0.3104856f;
    b4 = 0.55000f * b4 + white * 0.5329522f;
    b5 = -0.7616f * b5 - white * 0.0168980f;
    float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
    b6 = white * 0.115926f;
    return pink * 0.15f; // Normalized output
  }
};

/**
 * Single-pole Allpass filter
 * H(z) = (a + z^-1) / (1 + a * z^-1)
 */
class PhaserAllpass {
private:
  float z1 = 0.0f;

public:
  float process(float x, float a) {
    float y = a * x + z1;
    z1 = x - a * y;
    return y;
  }
};

/**
 * 8-stage Phaser
 * Architecture inspired by classic analog phasers.
 * Uses a modulated allpass chain with feedback and stereo width.
 */
class Phaser {
private:
  PhaserAllpass stagesL[12];
  PhaserAllpass stagesR[12];
  float feedbackL = 0.0f;
  float feedbackR = 0.0f;

  float lfoPhase = 0.0f;
  float targetFreq = 1.0f;
  float targetDepth = 0.5f;
  float currentFreq = 1.0f;
  float currentDepth = 0.5f;

  float targetNoiseGain = 0.0f;
  float currentNoiseGain = 0.0f;

  PinkNoise noiseL, noiseR;

public:
  void setParams(float k1, float k2, float noiseGain = 0.0f) {
    // K1: LFO Frequency (0.1Hz to 20Hz, Log scale)
    targetFreq =
        std::exp(std::log(0.1f) + k1 * (std::log(20.0f) - std::log(0.1f)));
    // K2: Depth (0.0 to 1.0)
    targetDepth = k2;
    targetNoiseGain = noiseGain;
  }

  void process(float &left, float &right, float sampleRate) {
    if (sampleRate < 1.0f)
      return;

    // Parameter Smoothing
    const float tau = 0.015f;
    float lambda = 1.0f - std::exp(-1.0f / (tau * sampleRate));
    currentFreq += (targetFreq - currentFreq) * lambda;
    currentDepth += (targetDepth - currentDepth) * lambda;
    currentNoiseGain += (targetNoiseGain - currentNoiseGain) * lambda;

    // LFO Update
    lfoPhase += currentFreq / sampleRate;
    while (lfoPhase > 1.0f)
      lfoPhase -= 1.0f;

    // Modulation Range: up to 4 octaves
    const float rangeOctaves = 4.0f * currentDepth;
    const float f_base = 800.0f; // Center base frequency

    // Left Channel LFO
    float lfoL = std::sin(2.0f * M_PI * lfoPhase);
    float fL = f_base * std::pow(2.0f, lfoL * rangeOctaves * 0.5f);
    fL = rack::math::clamp(fL, 20.0f, sampleRate * 0.45f);
    float tanL = std::tan(M_PI * fL / sampleRate);
    float aL = (1.0f - tanL) / (1.0f + tanL);

    // Right Channel LFO (90 degree phase shift for stereo width)
    float lfoR_val = std::sin(2.0f * M_PI * (lfoPhase + 0.25f));
    float fR = f_base * std::pow(2.0f, lfoR_val * rangeOctaves * 0.5f);
    fR = rack::math::clamp(fR, 20.0f, sampleRate * 0.45f);
    float tanR = std::tan(M_PI * fR / sampleRate);
    float aR = (1.0f - tanR) / (1.0f + tanR);

    // Feedback Amount (increased for 12 stages, up to 0.94)
    float fbAmount = 0.94f * currentDepth;

    // Inject Noise scaled by Depth and Master Noise Gain
    float noiseInject = currentNoiseGain * 0.4f;
    float nL = noiseL.process() * noiseInject;
    float nR = noiseR.process() * noiseInject;

    // Process Left
    float fbL = feedbackL * fbAmount;
    float inL = left + fbL + nL;
    float wetL = inL;
    for (int i = 0; i < 12; i++) {
      wetL = stagesL[i].process(wetL, aL);
    }
    feedbackL = wetL;

    // Process Right
    float fbR = feedbackR * fbAmount;
    float inR = right + fbR + nR;
    float wetR = inR;
    for (int i = 0; i < 12; i++) {
      wetR = stagesR[i].process(wetR, aR);
    }
    feedbackR = wetR;

    // Log-scale mapping for Dry/Wet mix to provide more resolution in the
    // useful phasing range Mix maps from 100% dry to 50/50 mix (max
    // cancellation)
    float mixTaper =
        currentDepth * currentDepth; // Simple quadratic log-like taper
    float mixRatio = mixTaper * 0.5f;
    float theta = (M_PI * 0.5f) * mixRatio;
    float gDry = std::cos(theta);
    float gWet = std::sin(theta);

    float outL = gDry * left + gWet * wetL;
    float outR = gDry * right + gWet * wetR;

    // Soft-clipping for stability with feedback
    float limit = 0.95f;
    for (float *s : {&outL, &outR}) {
      if (*s > limit)
        *s = limit + (1.0f - limit) * std::tanh((*s - limit) / (1.0f - limit));
      else if (*s < -limit)
        *s =
            -limit - (1.0f - limit) * std::tanh((-*s - limit) / (1.0f - limit));

      if (!std::isfinite(*s))
        *s = 0.0f;
    }

    left = outL;
    right = outR;
  }
};

} // namespace paisa
