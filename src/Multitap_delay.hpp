#pragma once
#include "FDNReverb.hpp"
#include "HoleReverbWrapper.hpp"
#include "Reverb.hpp"
#include "Tap.hpp"
#include "plugin.hpp"

struct Multitap_delay : Module {
  enum ParamId {
    ENUMS(MODE_PARAMS, 5),
    // 5 Columns (4 Taps + 1 Meta), each with 2 knobs
    ENUMS(COL_KNOB1_PARAMS, 5),
    ENUMS(COL_KNOB2_PARAMS, 5),
    INPUT_GAIN_PARAM,
    REVERB_MIX_PARAM,
    REVERB_GRAVITY_PARAM,
    REVERB_DIFFUSION_PARAM,
    REVERB_MODE_PARAM, // 0 for Default, 1 for FDN, 2 for Hole
    REVERB_DAMPING_PARAM,
    REVERB_MOD_FREQ_PARAM,
    REVERB_MOD_DEPTH_PARAM,
    REVERB_TIME_PARAM,
    PHASER_NOISE_GAIN_PARAM,
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
  float inputGainState = 0.923076f; // Default to 0dB

  float reverbMixState = 0.3f;
  float reverbGravityState = 0.5f;
  float reverbDiffusionState = 0.5f;
  float reverbDampingState = 0.2f;
  float reverbModFreqState = 0.5f;
  float reverbModDepthState = 0.5f;
  float reverbTimeState = 0.5f;
  float phaserNoiseGainState = 0.0f;

  int currentMode = 0;

  static constexpr size_t MAX_DELAY_SAMPLES = 192000 * 10;

  std::vector<std::unique_ptr<paisa::Tap>> taps;
  std::unique_ptr<paisa::Reverb> reverb;
  std::unique_ptr<paisa::FDNReverb> fdnReverb;
  std::unique_ptr<paisa::HoleReverb> holeReverb;

  int reverbMode = 0; // 0 for Default, 1 for FDN, 2 for Hole

  Multitap_delay();
  ~Multitap_delay();

  void process(const ProcessArgs &args) override;
  void onSampleRateChange() override;

  void updateKnobsFromState();

  json_t *dataToJson() override;
  void dataFromJson(json_t *rootJ) override;
};
