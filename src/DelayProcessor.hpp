#pragma once
#include "Processor.hpp"
#include <algorithm>

namespace paisa {

class DelayProcessor : public Processor {
  float *bufferL;
  float *bufferR;
  size_t bufferSize;
  size_t *writeIndex;

  float delayTime = 0.5f;
  float feedback = 0.0f;
  float offsetTime = 0.0f;
  float offsetFeedback = 0.0f;

  float readBuffer(float *buffer, float delaySamples, size_t wIdx) {
    if (delaySamples < 1.0f)
      delaySamples = 1.0f;
    if (delaySamples > bufferSize - 3)
      delaySamples = bufferSize - 3;

    float readPos = (float)wIdx - delaySamples;
    while (readPos < 0)
      readPos += bufferSize;

    int i1 = (int)readPos;
    int i0 = (i1 - 1 + bufferSize) % bufferSize;
    int i2 = (i1 + 1) % bufferSize;
    int i3 = (i1 + 2) % bufferSize;

    float frac = readPos - (float)i1;
    float y0 = buffer[i0], y1 = buffer[i1], y2 = buffer[i2], y3 = buffer[i3];

    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
  }

public:
  DelayProcessor(float *bL, float *bR, size_t size, size_t *wIdx)
      : bufferL(bL), bufferR(bR), bufferSize(size), writeIndex(wIdx) {}

  void setParams(float p1, float p2) override {
    // We'll interpret p1 as the normalized first knob, p2 as the second
    delayTime = p1 * 9.999f + 0.001f;
    feedback = p2;
  }

  void setOffsets(float o1, float o2) override {
    offsetTime = o1 * 5.0f; // metaDelayLimit was 5.0
    offsetFeedback = o2;
  }

  void process(float &left, float &right, float sampleRate) override {
    float totalTime = std::max(0.001f, delayTime + offsetTime);
    float totalFeedback =
        std::max(0.0f, std::min(1.0f, feedback + offsetFeedback));

    float tapL = readBuffer(bufferL, totalTime * sampleRate, *writeIndex);
    float tapR = readBuffer(bufferR, totalTime * sampleRate, *writeIndex);

    bufferL[*writeIndex] += tapL * totalFeedback;
    bufferR[*writeIndex] += tapR * totalFeedback;

    left = tapL;
    right = tapR;
  }
};

} // namespace paisa
