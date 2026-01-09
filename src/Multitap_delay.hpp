#pragma once
#include "Tap.hpp"
#include "plugin.hpp"

struct Multitap_delay : Module {
  enum ParamId {
    // 5 Global Mode buttons
    ENUMS(MODE_PARAMS, 5),
    // 5 Columns (4 Taps + 1 Meta), each with 2 knobs
    ENUMS(COL_KNOB1_PARAMS, 5),
    ENUMS(COL_KNOB2_PARAMS, 5),
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
  float knobState[5][5][2];
  int currentMode = 0;

  static constexpr size_t MAX_DELAY_SAMPLES = 192000 * 10;
  float *bufferL = nullptr;
  float *bufferR = nullptr;
  size_t writeIndex = 0;

  std::vector<std::unique_ptr<paisa::Tap>> taps;

  Multitap_delay();
  ~Multitap_delay();

  void onSampleRateChange() override;
  void process(const ProcessArgs &args) override;

  json_t *dataToJson() override;
  void dataFromJson(json_t *rootJ) override;
};
