#pragma once
#include "ReverbSupport.hpp"
#include <cmath>
#include <memory>
#include <vector>

namespace paisa {

class Reverb {
private:
  std::vector<std::unique_ptr<Allpass>> monoStages;
  std::vector<std::unique_ptr<Allpass>> leftStages;
  std::vector<std::unique_ptr<Allpass>> rightStages;

  DCBlocker dcBlockerL;
  DCBlocker dcBlockerR;

  float lfoPhase = 0.0f;
  float lfoOffsets[32];
  float baseDelays[32];

  // Target parameters
  float targetG = 0.5f;
  float targetDiffusion = 0.5f;
  float targetMix = 0.3f;

  // Smoothed parameters
  float currentG = 0.5f;
  float currentDiffusion = 0.5f;
  float currentMix = 0.3f;

  // Smoothing coefficient
  const float slewing = 0.001f;

  const int primes[32] = {
      151,  197,  251,  313,  389,  443,  503,  593,  // Mono
      701,  821,  941,  1061, 1181, 1301, 1423, 1543, // Left
      1667, 1787, 1907, 2027, 2141, 1279, 1151, 1031,
      709,  827,  947,  1063, 1187, 1303, 1427, 1549 // Right
  };

public:
  Reverb() {
    for (int i = 0; i < 8; i++) {
      monoStages.push_back(std::unique_ptr<Allpass>(new Allpass(4000)));
      lfoOffsets[i] = (float)i / 32.0f;
      baseDelays[i] = (float)primes[i];
    }
    for (int i = 0; i < 12; i++) {
      leftStages.push_back(std::unique_ptr<Allpass>(new Allpass(8000)));
      lfoOffsets[8 + i] = (float)(8 + i) / 32.0f;
      baseDelays[8 + i] = (float)primes[8 + i];
    }
    for (int i = 0; i < 12; i++) {
      rightStages.push_back(std::unique_ptr<Allpass>(new Allpass(8000)));
      lfoOffsets[20 + i] = (float)(20 + i) / 32.0f;
      baseDelays[20 + i] = (float)primes[20 + i];
    }
  }

  void setParams(float mixVal, float gravity, float diff) {
    targetMix = mixVal;
    targetDiffusion = diff;

    // Improved Gravity Mapping: more aggressive decay growth
    if (gravity < 0.4f) {
      // [0.0 - 0.4]: g scales from 0.1 to 0.5 (Small spaces)
      targetG = 0.1f + (gravity / 0.4f) * 0.4f;
    } else if (gravity < 0.75f) {
      // [0.4 - 0.75]: g scales from 0.5 to 0.8 (Large rooms)
      float t = (gravity - 0.4f) / 0.35f;
      targetG = 0.5f + t * 0.3f;
    } else {
      // [0.75 - 1.0]: g scales from 0.8 to 0.995 (Massive)
      float t = (gravity - 0.75f) / 0.25f;
      targetG = 0.8f + t * (0.995f - 0.8f);
    }
  }

  void process(float &left, float &right, float sampleRate) {
    float dryL = left;
    float dryR = right;

    // Smooth parameters sample-by-sample to avoid clicks
    currentG += (targetG - currentG) * slewing;
    currentDiffusion += (targetDiffusion - currentDiffusion) * slewing;
    currentMix += (targetMix - currentMix) * slewing;

    // Apply smoothed feedback to all stages
    for (auto &stage : monoStages)
      stage->setFeedback(currentG);
    for (auto &stage : leftStages)
      stage->setFeedback(currentG);
    for (auto &stage : rightStages)
      stage->setFeedback(currentG);

    float mono = (left + right) * 0.5f;

    // LFO Update
    lfoPhase += 0.5f / sampleRate;
    if (lfoPhase > 1.0f)
      lfoPhase -= 1.0f;

    // Smoothed diffusion scales delay times
    float diffScale = 0.1f + currentDiffusion * 4.0f;

    // Processing Mono Diffusion
    for (int i = 0; i < 8; i++) {
      float lfo = std::sin((lfoPhase + lfoOffsets[i]) * 2.0f * M_PI);
      float dTime = baseDelays[i] * diffScale + lfo * 10.0f;
      mono = monoStages[i]->process(mono, dTime);
    }

    float branchL = mono;
    float branchR = mono;

    // Stereo Branches
    for (int i = 0; i < 12; i++) {
      float lfoL = std::sin((lfoPhase + lfoOffsets[8 + i]) * 2.0f * M_PI);
      float dTimeL = baseDelays[8 + i] * diffScale + lfoL * 15.0f;
      branchL = leftStages[i]->process(branchL, dTimeL);

      float lfoR = std::sin((lfoPhase + lfoOffsets[20 + i]) * 2.0f * M_PI);
      float dTimeR = baseDelays[20 + i] * (diffScale * 1.08f) + lfoR * 15.0f;
      branchR = rightStages[i]->process(branchR, dTimeR);
    }

    // Output Mix
    left = dryL + currentMix * dcBlockerL.process(branchL);
    right = dryR + currentMix * dcBlockerR.process(branchR);
  }
};

} // namespace paisa
