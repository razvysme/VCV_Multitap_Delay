#pragma once
#include "Processor.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace paisa {

class DelayProcessor {
  std::vector<float> bufferL;
  std::vector<float> bufferR;
  size_t writeIndex = 0;

  float delayTimeParam = 0.5f;
  float feedbackParam = 0.0f;

  float actualTime = 0.5f;
  bool dirty = true;

  float readBuffer(const std::vector<float> &buffer, float delaySamples) {
    size_t size = buffer.size();
    if (delaySamples < 1.0f)
      delaySamples = 1.0f;
    if (delaySamples > (float)size - 3.0f)
      delaySamples = (float)size - 3.0f;

    float readPos = (float)writeIndex - delaySamples;
    while (readPos < 0)
      readPos += (float)size;

    int i1 = (int)readPos;
    int i2 = (i1 + 1) % size;
    float frac = readPos - (float)i1;

    return buffer[i1] * (1.0f - frac) + buffer[i2] * frac;
  }

public:
  DelayProcessor(size_t size) {
    bufferL.resize(size, 0.0f);
    bufferR.resize(size, 0.0f);
  }

  void setParams(float p1, float p2) {
    if (p1 != delayTimeParam || p2 != feedbackParam) {
      delayTimeParam = p1;
      feedbackParam = p2;
      dirty = true;
    }
  }

  float getFeedbackAmount() const { return feedbackParam; }

  void read(float &left, float &right, float sampleRate) {
    if (dirty) {
      float k1 = std::max(0.0f, std::min(1.0f, delayTimeParam));
      actualTime = std::exp(std::log(0.001f) +
                            k1 * (std::log(10.0f) - std::log(0.001f)));
      dirty = false;
    }
    left = readBuffer(bufferL, actualTime * sampleRate);
    right = readBuffer(bufferR, actualTime * sampleRate);
  }

  void write(float left, float right) {
    bufferL[writeIndex] = left;
    bufferR[writeIndex] = right;
    writeIndex++;
    if (writeIndex >= bufferL.size())
      writeIndex = 0;
  }
};

} // namespace paisa
