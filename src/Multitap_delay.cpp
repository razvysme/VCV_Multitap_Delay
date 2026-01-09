#include "Multitap_delay.hpp"

// Constants for experimentation
float maxFeedback = 1.0f;
float metaDelayLimit = 5.0f;

Multitap_delay::Multitap_delay() {
  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  for (int m = 0; m < 5; m++) {
    configParam(MODE_PARAMS + m, 0.f, 1.f, (m == 0 ? 1.f : 0.f), "Mode Select");
  }
  for (int i = 0; i < 5; i++) {
    configParam(COL_KNOB1_PARAMS + i, 0.f, 1.f, 0.5f, "Value 1");
    configParam(COL_KNOB2_PARAMS + i, 0.f, 1.f, i == 4 ? 0.5f : 0.0f,
                "Value 2");
  }

  configInput(IN_L_INPUT, "Left Input");
  configInput(IN_R_INPUT, "Right Input");

  for (int i = 0; i < 4; i++) {
    configOutput(OUT1_L_OUTPUT + i * 2, string::f("Tap %d Left", i + 1));
    configOutput(OUT1_R_OUTPUT + i * 2, string::f("Tap %d Right", i + 1));
  }
  configOutput(SUM_L_OUTPUT, "Sum Left");
  configOutput(SUM_R_OUTPUT, "Sum Right");

  bufferL = new float[MAX_DELAY_SAMPLES]();
  bufferR = new float[MAX_DELAY_SAMPLES]();

  for (int i = 0; i < 4; i++) {
    taps.push_back(std::unique_ptr<paisa::Tap>(
        new paisa::Tap(bufferL, bufferR, MAX_DELAY_SAMPLES, &writeIndex)));
  }

  // Initialize internal state for all columns and modes
  for (int col = 0; col < 5; col++) {
    for (int m = 0; m < 5; m++) {
      // Default normalized knob positions
      knobState[col][m][0] = 0.5f;
      knobState[col][m][1] = (col == 4 ? 0.5f : 0.0f);

      // Update tap objects with initial values
      if (col < 4) {
        taps[col]->setParam(m, knobState[col][m][0], knobState[col][m][1]);
      }
    }
  }
}

Multitap_delay::~Multitap_delay() {
  delete[] bufferL;
  delete[] bufferR;
}

void Multitap_delay::onSampleRateChange() {
  // Taps might need reset if we had a reset method, but SVFs handle it via
  // setParams usually
}

void Multitap_delay::process(const ProcessArgs &args) {
  float inL = inputs[IN_L_INPUT].getVoltage();
  float inR =
      inputs[IN_R_INPUT].isConnected() ? inputs[IN_R_INPUT].getVoltage() : inL;

  // Radio button logic for global mode
  for (int m = 0; m < 5; m++) {
    if (params[MODE_PARAMS + m].getValue() > 0.5f) {
      if (currentMode != m) {
        params[MODE_PARAMS + currentMode].setValue(0.f);
        currentMode = m;
        // Recall physical knob positions for the new mode
        for (int col = 0; col < 5; col++) {
          params[COL_KNOB1_PARAMS + col].setValue(
              knobState[col][currentMode][0]);
          params[COL_KNOB2_PARAMS + col].setValue(
              knobState[col][currentMode][1]);
        }
      }
    }
  }

  if (params[MODE_PARAMS + currentMode].getValue() < 0.5f) {
    params[MODE_PARAMS + currentMode].setValue(1.f);
  }

  // Update tap parameters from knobs
  for (int col = 0; col < 5; col++) {
    knobState[col][currentMode][0] = params[COL_KNOB1_PARAMS + col].getValue();
    knobState[col][currentMode][1] = params[COL_KNOB2_PARAMS + col].getValue();

    if (col < 4) {
      // Update individual tap parameters
      taps[col]->setParam(currentMode, knobState[col][currentMode][0],
                          knobState[col][currentMode][1]);
      // Also update offsets from the meta column (col 4)
      for (int m = 0; m < 5; m++) {
        float o1 = knobState[4][m][0] * 2.0f - 1.0f; // Bipolar offset
        float o2 = knobState[4][m][1] * 2.0f - 1.0f;
        taps[col]->setOffset(m, o1, o2);
      }
    }
  }

  // Update global mode lights
  for (int m = 0; m < 5; m++) {
    lights[MODE_LIGHTS + m].setBrightness(currentMode == m ? 1.f : 0.f);
  }

  bufferL[writeIndex] = inL;
  bufferR[writeIndex] = inR;

  float sumL = 0.f, sumR = 0.f;

  for (int i = 0; i < 4; i++) {
    float tapL, tapR;
    taps[i]->process(tapL, tapR, args.sampleRate);

    outputs[OUT1_L_OUTPUT + i * 2].setVoltage(tapL);
    outputs[OUT1_R_OUTPUT + i * 2].setVoltage(tapR);

    sumL += tapL;
    sumR += tapR;
  }

  outputs[SUM_L_OUTPUT].setVoltage(sumL * 0.25f);
  outputs[SUM_R_OUTPUT].setVoltage(sumR * 0.25f);

  writeIndex++;
  if (writeIndex >= MAX_DELAY_SAMPLES)
    writeIndex = 0;
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
        if (col < 4) {
          taps[col]->setParam(m, knobState[col][m][0], knobState[col][m][1]);
        }
      }
    }
  }
  json_t *modeJ = json_object_get(rootJ, "currentMode");
  if (modeJ) {
    currentMode = json_integer_value(modeJ);
  }
}

struct Multitap_delayWidget : ModuleWidget {
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

      addParam(createParamCentered<RoundBlackKnob>(
          mm2px(Vec(x, 45)), module, Multitap_delay::COL_KNOB1_PARAMS + i));
      addParam(createParamCentered<RoundBlackKnob>(
          mm2px(Vec(x, 65)), module, Multitap_delay::COL_KNOB2_PARAMS + i));
      if (i < 4) {
        addOutput(createOutputCentered<ThemedPJ301MPort>(
            mm2px(Vec(x, 95)), module, Multitap_delay::OUT1_L_OUTPUT + i * 2));
        addOutput(createOutputCentered<ThemedPJ301MPort>(
            mm2px(Vec(x, 110)), module, Multitap_delay::OUT1_R_OUTPUT + i * 2));
      }
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
};

Model *modelMultitap =
    createModel<Multitap_delay, Multitap_delayWidget>("Multitap");
