#include "Multitap_delay.hpp"
#include <iomanip>
#include <sstream>

// Speed for the relative encoders
float ENCODER_SENSITIVITY = 0.0015f;

struct RelativeKnob : RoundBlackKnob {
  int col = 0;
  int k = 0; // 0 for Knob 1, 1 for Knob 2
  bool isInputGain = false;

  enum ReverbKnobType {
    NO_REVERB,
    REVERB_MIX,
    REVERB_GRAVITY,
    REVERB_DIFFUSION
  };
  ReverbKnobType reverbType = NO_REVERB;

  void onDragMove(const event::DragMove &e) override {
    auto *module = dynamic_cast<Multitap_delay *>(this->module);
    if (!module) {
      RoundBlackKnob::onDragMove(e);
      return;
    }

    float delta = -e.mouseDelta.y * ENCODER_SENSITIVITY;
    if (APP->window->getMods() & GLFW_MOD_SHIFT)
      delta *= 0.1f;

    if (isInputGain) {
      module->inputGainState += delta;
    } else if (reverbType == REVERB_MIX) {
      module->reverbMixState += delta;
    } else if (reverbType == REVERB_GRAVITY) {
      module->reverbGravityState += delta;
    } else if (reverbType == REVERB_DIFFUSION) {
      module->reverbDiffusionState += delta;
    } else if (col == 4) { // Meta Column
      float maxVal = -1e10f;
      float minVal = 1e10f;
      for (int i = 0; i < 4; i++) {
        float val = module->knobState[i][module->currentMode][k];
        if (val > maxVal)
          maxVal = val;
        if (val < minVal)
          minVal = val;
      }

      float effectiveDelta = delta;
      if (delta < 0.f) {
        if (maxVal <= 0.f)
          effectiveDelta = 0.f;
        else if (maxVal + delta < 0.f)
          effectiveDelta = -maxVal;
      } else if (delta > 0.f) {
        if (minVal >= 1.f)
          effectiveDelta = 0.f;
        else if (minVal + delta > 1.f)
          effectiveDelta = 1.f - minVal;
      }

      for (int i = 0; i < 4; i++) {
        module->knobState[i][module->currentMode][k] += effectiveDelta;
      }
      module->knobState[4][module->currentMode][k] += effectiveDelta;
    } else {
      module->knobState[col][module->currentMode][k] += delta;
    }

    module->updateKnobsFromState();
  }

  void onDoubleClick(const event::DoubleClick &e) override {
    auto *module = dynamic_cast<Multitap_delay *>(this->module);
    if (module) {
      if (isInputGain) {
        module->inputGainState = 0.923076f; // 0dB
      } else if (reverbType == REVERB_MIX) {
        module->reverbMixState = 0.3f;
      } else if (reverbType == REVERB_GRAVITY) {
        module->reverbGravityState = 0.5f;
      } else if (reverbType == REVERB_DIFFUSION) {
        module->reverbDiffusionState = 0.5f;
      } else {
        float def = 0.5f;
        if (module->currentMode == Multitap_delay::MODE_DELAY && k == 1)
          def = 0.0f;
        if (module->currentMode == Multitap_delay::MODE_FILTER && k == 1)
          def = 1.0f;

        if (col == 4) {
          for (int i = 0; i < 5; i++)
            module->knobState[i][module->currentMode][k] = def;
        } else {
          module->knobState[col][module->currentMode][k] = def;
        }
      }
      module->updateKnobsFromState();
    }
    RoundBlackKnob::onDoubleClick(e);
  }
};

Multitap_delay::Multitap_delay() {
  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  for (int m = 0; m < 5; m++) {
    configParam(MODE_PARAMS + m, 0.f, 1.f, (m == 0 ? 1.f : 0.f), "Mode Select");
  }

  for (int i = 0; i < 5; i++) {
    configParam(COL_KNOB1_PARAMS + i, 0.f, 1.f, 0.5f, "Value 1");
    configParam(COL_KNOB2_PARAMS + i, 0.f, 1.f, 0.5f, "Value 2");
  }

  configParam(INPUT_GAIN_PARAM, 0.f, 1.f, 0.923076f, "Input Gain");
  configParam(REVERB_MIX_PARAM, 0.f, 1.f, 0.3f, "Reverb Mix");
  configParam(REVERB_GRAVITY_PARAM, 0.f, 1.f, 0.5f, "Reverb Gravity");
  configParam(REVERB_DIFFUSION_PARAM, 0.f, 1.f, 0.5f, "Reverb Diffusion");

  configInput(IN_L_INPUT, "Left Input");
  configInput(IN_R_INPUT, "Right Input");

  for (int i = 0; i < 4; i++) {
    configOutput(OUT1_L_OUTPUT + i * 2, string::f("Tap %d Left", i + 1));
    configOutput(OUT1_R_OUTPUT + i * 2, string::f("Tap %d Right", i + 1));
  }
  configOutput(SUM_L_OUTPUT, "Sum Left");
  configOutput(SUM_R_OUTPUT, "Sum Right");

  for (int i = 0; i < 4; i++) {
    taps.push_back(
        std::unique_ptr<paisa::Tap>(new paisa::Tap(MAX_DELAY_SAMPLES)));
  }

  reverb = std::unique_ptr<paisa::Reverb>(new paisa::Reverb());

  for (int col = 0; col < 5; col++) {
    for (int m = 0; m < 5; m++) {
      knobState[col][m][0] = 0.5f;
      knobState[col][m][1] = 0.5f;
      if (m == MODE_DELAY) {
        knobState[col][m][0] = 0.05f + col * 0.05f;
        knobState[col][m][1] = 0.0f;
      } else if (m == MODE_AMP_PAN) {
        knobState[col][m][0] = 0.5f;
        knobState[col][m][1] = 0.5f;
      } else if (m == MODE_FILTER) {
        knobState[col][m][0] = 0.5f;
        knobState[col][m][1] = 1.0f;
      }

      if (col < 4) {
        taps[col]->setParam(m, knobState[col][m][0], knobState[col][m][1]);
      }
    }
  }
  updateKnobsFromState();
}

Multitap_delay::~Multitap_delay() {}

void Multitap_delay::updateKnobsFromState() {
  for (int i = 0; i < 5; i++) {
    params[COL_KNOB1_PARAMS + i].setValue(
        math::clamp(knobState[i][currentMode][0], 0.f, 1.f));
    params[COL_KNOB2_PARAMS + i].setValue(
        math::clamp(knobState[i][currentMode][1], 0.f, 1.f));

    if (i < 4) {
      for (int m = 0; m < NUM_MODES; m++) {
        taps[i]->setParam(m, knobState[i][m][0], knobState[i][m][1]);
      }
    }
  }
  params[INPUT_GAIN_PARAM].setValue(math::clamp(inputGainState, 0.f, 1.f));
  params[REVERB_MIX_PARAM].setValue(math::clamp(reverbMixState, 0.f, 1.f));
  params[REVERB_GRAVITY_PARAM].setValue(
      math::clamp(reverbGravityState, 0.f, 1.f));
  params[REVERB_DIFFUSION_PARAM].setValue(
      math::clamp(reverbDiffusionState, 0.f, 1.f));

  if (reverb) {
    reverb->setParams(math::clamp(reverbMixState, 0.f, 1.f),
                      math::clamp(reverbGravityState, 0.f, 1.f),
                      math::clamp(reverbDiffusionState, 0.f, 1.f));
  }
}

void Multitap_delay::onSampleRateChange() {}

void Multitap_delay::process(const ProcessArgs &args) {
  float inL = inputs[IN_L_INPUT].getVoltage();
  float inR =
      inputs[IN_R_INPUT].isConnected() ? inputs[IN_R_INPUT].getVoltage() : inL;

  float kIn = math::clamp(inputGainState, 0.f, 1.f);
  float gainInDb = kIn * 78.f - 72.f;
  float gainIn = std::pow(10.f, gainInDb / 20.f);
  inL *= gainIn;
  inR *= gainIn;

  for (int m = 0; m < 5; m++) {
    if (params[MODE_PARAMS + m].getValue() > 0.5f) {
      if (currentMode != m) {
        params[MODE_PARAMS + currentMode].setValue(0.f);
        currentMode = m;
        updateKnobsFromState();
      }
    }
  }
  if (params[MODE_PARAMS + currentMode].getValue() < 0.5f) {
    params[MODE_PARAMS + currentMode].setValue(1.f);
  }

  float sumL = 0.f, sumR = 0.f;
  for (int i = 0; i < 4; i++) {
    float tapL, tapR;
    taps[i]->process(inL, inR, tapL, tapR, args.sampleRate);
    outputs[OUT1_L_OUTPUT + i * 2].setVoltage(tapL);
    outputs[OUT1_R_OUTPUT + i * 2].setVoltage(tapR);
    sumL += tapL;
    sumR += tapR;
  }

  float outL = sumL * 0.25f;
  float outR = sumR * 0.25f;
  if (reverb)
    reverb->process(outL, outR, args.sampleRate);

  outputs[SUM_L_OUTPUT].setVoltage(outL);
  outputs[SUM_R_OUTPUT].setVoltage(outR);

  for (int m = 0; m < 5; m++) {
    lights[MODE_LIGHTS + m].setBrightness(currentMode == m ? 1.f : 0.f);
  }
}

json_t *Multitap_delay::dataToJson() {
  json_t *rootJ = json_object();
  json_t *stateA = json_array();
  for (int col = 0; col < 5; col++) {
    for (int m = 0; m < 5; m++) {
      json_array_append_new(stateA, json_real(knobState[col][m][0]));
      json_array_append_new(stateA, json_real(knobState[col][m][1]));
    }
  }
  json_object_set_new(rootJ, "knobState", stateA);
  json_object_set_new(rootJ, "inputGainState", json_real(inputGainState));
  json_object_set_new(rootJ, "reverbMixState", json_real(reverbMixState));
  json_object_set_new(rootJ, "reverbGravityState",
                      json_real(reverbGravityState));
  json_object_set_new(rootJ, "reverbDiffusionState",
                      json_real(reverbDiffusionState));
  json_object_set_new(rootJ, "currentMode", json_integer(currentMode));
  return rootJ;
}

void Multitap_delay::dataFromJson(json_t *rootJ) {
  json_t *stateA = json_object_get(rootJ, "knobState");
  if (stateA) {
    for (int col = 0; col < 5; col++) {
      for (int m = 0; m < 5; m++) {
        knobState[col][m][0] =
            json_real_value(json_array_get(stateA, col * 10 + m * 2));
        knobState[col][m][1] =
            json_real_value(json_array_get(stateA, col * 10 + m * 2 + 1));
      }
    }
  }
  json_t *igJ = json_object_get(rootJ, "inputGainState");
  if (igJ)
    inputGainState = json_real_value(igJ);
  json_t *rmJ = json_object_get(rootJ, "reverbMixState");
  if (rmJ)
    reverbMixState = json_real_value(rmJ);
  json_t *rgJ = json_object_get(rootJ, "reverbGravityState");
  if (rgJ)
    reverbGravityState = json_real_value(rgJ);
  json_t *rdJ = json_object_get(rootJ, "reverbDiffusionState");
  if (rdJ)
    reverbDiffusionState = json_real_value(rdJ);
  json_t *modeJ = json_object_get(rootJ, "currentMode");
  if (modeJ)
    currentMode = json_integer_value(modeJ);
  updateKnobsFromState();
}

struct Multitap_delayWidget : ModuleWidget {
  Label *knobLabels[5][2];
  Label *inputGainLabel;
  Label *reverbLabels[3];

  Multitap_delayWidget(Multitap_delay *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Multitap.svg")));
    addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(
        createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ThemedScrew>(
        Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ThemedScrew>(Vec(
        box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    float centerModeX = 45.0;
    for (int m = 0; m < 5; m++) {
      float btnX = centerModeX - 16.0 + m * 8.0;
      addParam(
          createLightParamCentered<VCVLightLatch<SmallSimpleLight<WhiteLight>>>(
              mm2px(Vec(btnX, 15)), module, Multitap_delay::MODE_PARAMS + m,
              Multitap_delay::MODE_LIGHTS + m));
    }
    for (int i = 0; i < 5; i++) {
      float x = 22.86 + i * 15.24;
      if (i == 4)
        x = 86.8;
      knobLabels[i][0] = new Label();
      knobLabels[i][0]->box.pos = mm2px(Vec(x - 7.5, 29));
      knobLabels[i][0]->box.size = mm2px(Vec(15, 7));
      knobLabels[i][0]->fontSize = 10;
      knobLabels[i][0]->color = nvgRGB(0xff, 0xff, 0xff);
      knobLabels[i][0]->text = "";
      addChild(knobLabels[i][0]);
      auto *knob1 = createParamCentered<RelativeKnob>(
          mm2px(Vec(x, 45)), module, Multitap_delay::COL_KNOB1_PARAMS + i);
      knob1->col = i;
      knob1->k = 0;
      addParam(knob1);
      auto *knob2 = createParamCentered<RelativeKnob>(
          mm2px(Vec(x, 65)), module, Multitap_delay::COL_KNOB2_PARAMS + i);
      knob2->col = i;
      knob2->k = 1;
      addParam(knob2);
      knobLabels[i][1] = new Label();
      knobLabels[i][1]->box.pos = mm2px(Vec(x - 7.5, 74));
      knobLabels[i][1]->box.size = mm2px(Vec(15, 7));
      knobLabels[i][1]->fontSize = 10;
      knobLabels[i][1]->color = nvgRGB(0xff, 0xff, 0xff);
      knobLabels[i][1]->text = "";
      addChild(knobLabels[i][1]);
      if (i < 4) {
        addOutput(createOutputCentered<ThemedPJ301MPort>(
            mm2px(Vec(x, 95)), module, Multitap_delay::OUT1_L_OUTPUT + i * 2));
        addOutput(createOutputCentered<ThemedPJ301MPort>(
            mm2px(Vec(x, 110)), module, Multitap_delay::OUT1_R_OUTPUT + i * 2));
      }
    }
    inputGainLabel = new Label();
    inputGainLabel->box.pos = mm2px(Vec(7.62 - 7.5, 23));
    inputGainLabel->box.size = mm2px(Vec(15, 7));
    inputGainLabel->fontSize = 10;
    inputGainLabel->color = nvgRGB(0xff, 0xff, 0xff);
    inputGainLabel->text = "0dB";
    addChild(inputGainLabel);
    auto *igKnob = createParamCentered<RelativeKnob>(
        mm2px(Vec(7.62, 35)), module, Multitap_delay::INPUT_GAIN_PARAM);
    igKnob->isInputGain = true;
    addParam(igKnob);
    for (int i = 0; i < 3; i++) {
      float x = 71.0 + i * 11.5;
      float y = 92.0;
      reverbLabels[i] = new Label();
      reverbLabels[i]->box.pos = mm2px(Vec(x - 6.0, y - 8));
      reverbLabels[i]->box.size = mm2px(Vec(12, 6));
      reverbLabels[i]->fontSize = 8;
      reverbLabels[i]->color = nvgRGB(0xff, 0xff, 0xff);
      reverbLabels[i]->text = "";
      addChild(reverbLabels[i]);
      auto *rk = createParamCentered<RelativeKnob>(
          mm2px(Vec(x, y)), module, Multitap_delay::REVERB_MIX_PARAM + i);
      if (i == 0)
        rk->reverbType = RelativeKnob::REVERB_MIX;
      if (i == 1)
        rk->reverbType = RelativeKnob::REVERB_GRAVITY;
      if (i == 2)
        rk->reverbType = RelativeKnob::REVERB_DIFFUSION;
      addParam(rk);
    }
    addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(7.62, 50)), module,
                                                   Multitap_delay::IN_L_INPUT));
    addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(7.62, 65)), module,
                                                   Multitap_delay::IN_R_INPUT));
    addOutput(createOutputCentered<ThemedPJ301MPort>(
        mm2px(Vec(95.0, 95)), module, Multitap_delay::SUM_L_OUTPUT));
    addOutput(createOutputCentered<ThemedPJ301MPort>(
        mm2px(Vec(95.0, 110)), module, Multitap_delay::SUM_R_OUTPUT));
  }

  void step() override {
    auto *module = dynamic_cast<Multitap_delay *>(this->module);
    if (!module)
      return;
    for (int i = 0; i < 5; i++) {
      for (int k = 0; k < 2; k++) {
        float val = module->knobState[i][module->currentMode][k];
        float clamped = math::clamp(val, 0.f, 1.f);
        knobLabels[i][k]->text = formatValue(module->currentMode, k, clamped);
      }
    }
    float kIn = math::clamp(module->inputGainState, 0.f, 1.f);
    float dbIn = kIn * 78.f - 72.f;
    std::stringstream ssIn;
    ssIn << std::fixed << std::setprecision(1) << dbIn << "dB";
    inputGainLabel->text = ssIn.str();
    std::stringstream ssM;
    ssM << (int)(math::clamp(module->reverbMixState, 0.f, 1.f) * 100.f) << "%";
    reverbLabels[0]->text = ssM.str();
    std::stringstream ssG;
    ssG << "G:" << std::fixed << std::setprecision(2)
        << math::clamp(module->reverbGravityState, 0.f, 1.f);
    reverbLabels[1]->text = ssG.str();
    std::stringstream ssD;
    ssD << "D:" << std::fixed << std::setprecision(2)
        << math::clamp(module->reverbDiffusionState, 0.f, 1.f);
    reverbLabels[2]->text = ssD.str();
    ModuleWidget::step();
  }

  std::string formatValue(int mode, int k, float val) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);
    switch (mode) {
    case Multitap_delay::MODE_DELAY:
      if (k == 0) {
        float time = std::exp(std::log(0.001f) +
                              val * (std::log(10.0f) - std::log(0.001f)));
        if (time < 1.0f)
          ss << (time * 1000.f) << "ms";
        else
          ss << time << "s";
      } else
        ss << (val * 100.f) << "%";
      break;
    case Multitap_delay::MODE_AMP_PAN:
      if (k == 0)
        ss << (val * 78.f - 72.f) << "dB";
      else
        ss << (val * 2.f - 1.f)
           << (val < 0.5f ? "L" : (val > 0.5f ? "R" : "C"));
      break;
    case Multitap_delay::MODE_FILTER:
      if (k == 0) {
        float f = std::exp(std::log(20.f) +
                           val * (std::log(20000.f) - std::log(20.f)));
        if (f < 1000.f)
          ss << (int)f << "Hz";
        else
          ss << (f / 1000.f) << "kHz";
      } else
        ss << (val * 10.f) << "oct";
      break;
    default:
      ss << std::setprecision(2) << val;
      break;
    }
    return ss.str();
  }
};

Model *modelMultitap =
    createModel<Multitap_delay, Multitap_delayWidget>("Multitap");
