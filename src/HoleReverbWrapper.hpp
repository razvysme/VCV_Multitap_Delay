#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

// Faust-compatible stubs
struct Meta {
  virtual void declare(const char *key, const char *value) = 0;
  virtual ~Meta() {}
};

struct UI {
  virtual void openVerticalBox(const char *label) = 0;
  virtual void openHorizontalBox(const char *label) = 0;
  virtual void closeBox() = 0;
  virtual void addHorizontalSlider(const char *label, float *zone, float init,
                                   float min, float max, float step) = 0;
  virtual ~UI() {}
};

struct dsp {
  virtual ~dsp() {}
  virtual int getNumInputs() = 0;
  virtual int getNumOutputs() = 0;
  virtual void buildUserInterface(UI *ui_interface) = 0;
  virtual void init(int sample_rate) = 0;
  virtual void compute(int count, float **inputs, float **outputs) = 0;
};

#define FAUSTFLOAT float
#include "HoleReverb.hpp"

namespace paisa {

class HoleReverb {
public:
  // --- HOLE REVERB TUNING PARAMETERS ---
  float delayTime = 0.2f;
  float damping = 0.135f;
  float feedback = 0.3f;
  float modDepth = 0.1f;
  float modFreq = 2.0f;
  // -------------------------------------

  HoleReverb() { inner = new mydsp(); }

  ~HoleReverb() { delete inner; }

  void setParams(float mix, float sizeKnob, float diffusionKnob) {
    this->mix = mix;
    // Mapping size: knob 0..1 -> 0.5..3.0
    float size = 0.5f + (sizeKnob * 2.5f);
    // Mapping diffusion: knob 0..1 -> 0.1..1.0
    float diffusion = 0.1f + (diffusionKnob * 0.9f);

    // Map values to internal Faust sliders
    // Note: mydsp slider members are public (fHslider0 to fHslider6)
    // damping: fHslider0
    // feedback: fHslider1
    // modFreq: fHslider2
    // modDepth: fHslider3
    // delayTime: fHslider4
    // diffusion: fHslider5
    // size: fHslider6

    inner->fHslider0 = damping;
    inner->fHslider1 = feedback;
    inner->fHslider2 = modFreq;
    inner->fHslider3 = modDepth;
    inner->fHslider4 = delayTime;
    inner->fHslider5 = diffusion;
    inner->fHslider6 = size;
  }

  void process(float &left, float &right, float sampleRate) {
    if (std::abs(sampleRate - lastSampleRate) > 1.0f) {
      inner->init((int)sampleRate);
      lastSampleRate = sampleRate;
    }

    float *inputs[2] = {&left, &right};
    float outL = 0.f, outR = 0.f;
    float *outputs[2] = {&outL, &outR};

    inner->compute(1, inputs, outputs);

    // Apply mix
    left = left + (outL - left) * mix;
    right = right + (outR - right) * mix;

    if (!std::isfinite(left))
      left = 0.0f;
    if (!std::isfinite(right))
      right = 0.0f;
  }

private:
  mydsp *inner;
  float mix = 0.3f;
  float lastSampleRate = 0.0f;
};

} // namespace paisa
