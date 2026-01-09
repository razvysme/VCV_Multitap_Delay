#include "Multitap_delay.hpp"

// Constants for experimentation
float maxFeedback = 1.0f;
float metaDelayLimit = 5.0f;

// Hex colors for modes (Variables for easy experimentation)
uint32_t colorDelayHex = 0x33cc33;
uint32_t colorAmpHex = 0x3366ff;
uint32_t colorFiltHex = 0xff9900;
uint32_t colorFX1Hex = 0xcc33ff;
uint32_t colorFX2Hex = 0xff3333;

Multitap_delay::Multitap_delay() {
  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  for (int i = 0; i < 4; i++) {
    configParam(TAP1_MODE_PARAM + i * 3, 0.f, 4.f, 0.f, "Tap Mode");
    configParam(TAP1_KNOB1_PARAM + i * 3, 0.f, 1.f, 0.5f, "Value 1");
    configParam(TAP1_KNOB2_PARAM + i * 3, 0.f, 1.f, 0.0f, "Value 2");
  }

  configParam(META_MODE_PARAM, 0.f, 4.f, 0.f, "Meta Mode");
  configParam(META_KNOB1_PARAM, 0.f, 1.f, 0.5f, "Meta Offset 1");
  configParam(META_KNOB2_PARAM, 0.f, 1.f, 0.5f, "Meta Offset 2");

  configInput(IN_L_INPUT, "Left Input");
  configInput(IN_R_INPUT, "Right Input");

  for (int i = 0; i < 4; i++) {
    configOutput(OUT1_L_OUTPUT + i * 2, string::f("Tap %d Left", i + 1));
    configOutput(OUT1_R_OUTPUT + i * 2, string::f("Tap %d Right", i + 1));
  }
  configOutput(SUM_L_OUTPUT, "Sum Left");
  configOutput(SUM_R_OUTPUT, "Sum Right");

  // Initialize internal state
  for (int col = 0; col < 5; col++) {
    state[col][MODE_DELAY][0] = 0.5f;
    state[col][MODE_DELAY][1] = 0.0f;
    state[col][MODE_AMP_PAN][0] = 0.0f;
    state[col][MODE_AMP_PAN][1] = 0.0f;
    state[col][MODE_FILTER][0] = 400.0f;
    state[col][MODE_FILTER][1] = 50.0f;
    state[col][MODE_FX1][0] = 0.0f;
    state[col][MODE_FX1][1] = 0.0f;
    state[col][MODE_FX2][0] = 0.0f;
    state[col][MODE_FX2][1] = 0.0f;
  }

  bufferL = new float[MAX_DELAY_SAMPLES]();
  bufferR = new float[MAX_DELAY_SAMPLES]();
}

Multitap_delay::~Multitap_delay() {
  delete[] bufferL;
  delete[] bufferR;
}

void Multitap_delay::onSampleRateChange() {
  for (int i = 0; i < 4; i++) {
    filterL[i].reset();
    filterR[i].reset();
  }
}

float Multitap_delay::readBuffer(float *buffer, float delaySamples) {
  if (delaySamples < 1.0f)
    delaySamples = 1.0f;
  if (delaySamples > MAX_DELAY_SAMPLES - 3)
    delaySamples = MAX_DELAY_SAMPLES - 3;
  float readPos = (float)writeIndex - delaySamples;
  while (readPos < 0)
    readPos += MAX_DELAY_SAMPLES;
  int i1 = (int)readPos;
  int i0 = (i1 - 1 + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
  int i2 = (i1 + 1) % MAX_DELAY_SAMPLES;
  int i3 = (i1 + 2) % MAX_DELAY_SAMPLES;
  float frac = readPos - (float)i1;
  float y0 = buffer[i0], y1 = buffer[i1], y2 = buffer[i2], y3 = buffer[i3];
  float c0 = y1, c1 = 0.5f * (y2 - y0);
  float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
  float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
  return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void Multitap_delay::process(const ProcessArgs &args) {
  float inL = inputs[IN_L_INPUT].getVoltage();
  float inR =
      inputs[IN_R_INPUT].isConnected() ? inputs[IN_R_INPUT].getVoltage() : inL;

  for (int col = 0; col < 5; col++) {
    int mode = (int)std::round(params[TAP1_MODE_PARAM + col * 3].getValue());
    mode = clamp(mode, 0, (int)NUM_MODES - 1);
    float k1 = params[TAP1_KNOB1_PARAM + col * 3].getValue();
    float k2 = params[TAP1_KNOB2_PARAM + col * 3].getValue();

    switch (mode) {
    case MODE_DELAY:
      if (col < 4) {
        state[col][MODE_DELAY][0] = k1 * 9.999f + 0.001f;
        state[col][MODE_DELAY][1] = k2 * maxFeedback;
      } else {
        state[col][MODE_DELAY][0] = (k1 * 2.f - 1.f) * metaDelayLimit;
        state[col][MODE_DELAY][1] = (k2 * 2.f - 1.f);
      }
      break;
    case MODE_AMP_PAN:
      state[col][MODE_AMP_PAN][0] = k1 * 78.f - 72.f;
      state[col][MODE_AMP_PAN][1] = k2 * 2.f - 1.f;
      break;
    case MODE_FILTER:
      if (col < 4) {
        state[col][MODE_FILTER][0] = std::exp(
            std::log(20.f) + k1 * (std::log(20000.f) - std::log(20.f)));
        state[col][MODE_FILTER][1] = k2 * 100.f;
      } else {
        state[col][MODE_FILTER][0] = (k1 * 2.f - 1.f) * 10000.f;
        state[col][MODE_FILTER][1] = (k2 * 2.f - 1.f) * 50.f;
      }
      break;
    default:
      state[col][mode][0] = k1;
      state[col][mode][1] = k2;
      break;
    }

    // Update button lights
    for (int m = 0; m < 5; m++) {
      lights[MODE_LIGHTS + col * 5 + m].setBrightness(mode == m ? 1.f : 0.f);
    }
  }

  bufferL[writeIndex] = inL;
  bufferR[writeIndex] = inR;

  float sumL = 0.f, sumR = 0.f;
  float mTime = state[4][MODE_DELAY][0];
  float mFeed = state[4][MODE_DELAY][1];
  float mAmp = state[4][MODE_AMP_PAN][0];
  float mPan = state[4][MODE_AMP_PAN][1];
  float mBase = state[4][MODE_FILTER][0];
  float mWidth = state[4][MODE_FILTER][1];

  for (int i = 0; i < 4; i++) {
    float delayTime = clamp(state[i][MODE_DELAY][0] + mTime, 0.001f, 10.f);
    float feedback = clamp(state[i][MODE_DELAY][1] + mFeed, 0.f, maxFeedback);
    float tapRawL = readBuffer(bufferL, delayTime * args.sampleRate);
    float tapRawR = readBuffer(bufferR, delayTime * args.sampleRate);

    bufferL[writeIndex] += tapRawL * feedback;
    bufferR[writeIndex] += tapRawR * feedback;

    float db = clamp(state[i][MODE_AMP_PAN][0] + mAmp, -100.f, 24.f);
    float gain = std::pow(10.f, db / 20.f);
    float pan = clamp(state[i][MODE_AMP_PAN][1] + mPan, -1.f, 1.f);
    float panL = std::cos((pan + 1.f) * M_PI / 4.f);
    float panR = std::sin((pan + 1.f) * M_PI / 4.f);

    float processedL = tapRawL * gain * panL;
    float processedR = tapRawR * gain * panR;

    float base = clamp(state[i][MODE_FILTER][0] + mBase, 20.f, 20000.f);
    float width = clamp(state[i][MODE_FILTER][1] + mWidth, 0.f, 100.f);
    filterL[i].setParams(args.sampleRate, base, width);
    filterR[i].setParams(args.sampleRate, base, width);

    processedL = filterL[i].process(processedL);
    processedR = filterR[i].process(processedR);

    outputs[OUT1_L_OUTPUT + i * 2].setVoltage(processedL);
    outputs[OUT1_R_OUTPUT + i * 2].setVoltage(processedR);

    sumL += processedL;
    sumR += processedR;
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
      json_array_append_new(stateA, json_real(state[col][m][0]));
      json_array_append_new(stateA, json_real(state[col][m][1]));
    }
  }
  json_object_set_new(rootJ, "state", stateA);
  return rootJ;
}

void Multitap_delay::dataFromJson(json_t *rootJ) {
  json_t *stateA = json_object_get(rootJ, "state");
  if (stateA) {
    for (int col = 0; col < 5; col++) {
      for (int m = 0; m < 5; m++) {
        state[col][m][0] =
            json_real_value(json_array_get(stateA, col * 10 + m * 2));
        state[col][m][1] =
            json_real_value(json_array_get(stateA, col * 10 + m * 2 + 1));
      }
    }
  }
}

// Custom Radio Button with light (Mutes.cpp style)
struct ModeButton : app::SvgSwitch {
  int modeValue = 0;
  ModeButton() {
    addFrame(
        Svg::load(asset::system("res/ComponentLibrary/RoundButton_0.svg")));
    addFrame(
        Svg::load(asset::system("res/ComponentLibrary/RoundButton_1.svg")));
  }
  void onButton(const event::Button &e) override {
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
      if (getParamQuantity())
        getParamQuantity()->setValue((float)modeValue);
    }
  }
};

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

    uint32_t colors[5] = {colorDelayHex, colorAmpHex, colorFiltHex, colorFX1Hex,
                          colorFX2Hex};

    for (int i = 0; i < 5; i++) {
      float x = 22.86 + i * 15.24;
      if (i == 4)
        x = 86.8;
      // 5 Mode Buttons
      for (int m = 0; m < 5; m++) {
        float btnX = x - 7.0 + m * 3.5;
        auto *btn = createParamCentered<ModeButton>(
            mm2px(Vec(btnX, 20)), module,
            Multitap_delay::TAP1_MODE_PARAM + i * 3);
        btn->modeValue = m;
        addParam(btn);

        // Add a light to the button
        auto *light = createLightCentered<SmallSimpleLight<WhiteLight>>(
            mm2px(Vec(btnX, 20)), module,
            Multitap_delay::MODE_LIGHTS + i * 5 + m);
        light->addBaseColor(nvgRGB((colors[m] >> 16) & 0xff,
                                   (colors[m] >> 8) & 0xff, colors[m] & 0xff));
        addChild(light);
      }
      addParam(createParamCentered<RoundBlackKnob>(
          mm2px(Vec(x, 45)), module, Multitap_delay::TAP1_KNOB1_PARAM + i * 3));
      addParam(createParamCentered<RoundBlackKnob>(
          mm2px(Vec(x, 65)), module, Multitap_delay::TAP1_KNOB2_PARAM + i * 3));
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
