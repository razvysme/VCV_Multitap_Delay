#pragma once
#include "Filter.hpp"
#include "plugin.hpp"

struct Multitap_delay : Module {
  enum ParamId {
    // 4 Taps
    TAP1_MODE_PARAM,
    TAP1_KNOB1_PARAM,
    TAP1_KNOB2_PARAM,
    TAP2_MODE_PARAM,
    TAP2_KNOB1_PARAM,
    TAP2_KNOB2_PARAM,
    TAP3_MODE_PARAM,
    TAP3_KNOB1_PARAM,
    TAP3_KNOB2_PARAM,
    TAP4_MODE_PARAM,
    TAP4_KNOB1_PARAM,
    TAP4_KNOB2_PARAM,
    // Meta Column
    META_MODE_PARAM,
    META_KNOB1_PARAM,
    META_KNOB2_PARAM,
    NUM_PARAMS
  };
  enum InputId { IN_L_INPUT, IN_R_INPUT, NUM_INPUTS };
  enum OutputId {
    OUT1_L_OUTPUT,
    OUT1_R_OUTPUT,
    OUT2_L_OUTPUT,
    OUT2_R_OUTPUT,
    OUT3_L_OUTPUT,
    OUT3_R_OUTPUT,
    OUT4_L_OUTPUT,
    OUT4_R_OUTPUT,
    SUM_L_OUTPUT,
    SUM_R_OUTPUT,
    NUM_OUTPUTS
  };
  enum LightId {
    ENUMS(MODE_LIGHTS, 25), // 5 columns * 5 modes
    NUM_LIGHTS
  };

  enum Mode {
    MODE_DELAY,   // Delay Time, Feedback
    MODE_AMP_PAN, // Amplitude, Panning
    MODE_FILTER,  // Base, Width
    MODE_FX1,     // Placeholder
    MODE_FX2,     // Placeholder
    NUM_MODES
  };

  // Internal State Storage (for persistence and mode switching)
  // [Tap Index 0-4 (4 is Meta)][Mode Index][Knob 0-1]
  float state[5][5][2];

  static constexpr size_t MAX_DELAY_SAMPLES = 192000 * 10;
  float *bufferL = nullptr;
  float *bufferR = nullptr;
  size_t writeIndex = 0;

  paisa::BaseWidthFilter filterL[4];
  paisa::BaseWidthFilter filterR[4];

  Multitap_delay();
  ~Multitap_delay();

  void onSampleRateChange() override;
  void process(const ProcessArgs &args) override;
  float readBuffer(float *buffer, float delaySamples);

  json_t *dataToJson() override;
  void dataFromJson(json_t *rootJ) override;
};
