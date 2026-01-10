#pragma once
#include <algorithm>
#include <vector>

namespace paisa {

class Allpass {
private:
  std::vector<float> buffer;
  int writeIndex = 0;
  float g = 0.5f;

public:
  Allpass(int size) { buffer.resize(size, 0.0f); }

  void setFeedback(float feedback) { g = feedback; }

  float process(float x, float delaySamples) {
    float size = (float)buffer.size();
    if (delaySamples < 1.0f)
      delaySamples = 1.0f;
    if (delaySamples > size - 2.0f)
      delaySamples = size - 2.0f;

    float readPos = (float)writeIndex - delaySamples;
    while (readPos < 0.0f)
      readPos += size;

    int i1 = (int)readPos;
    int i2 = (i1 + 1) % buffer.size();
    float frac = readPos - (float)i1;

    // Linear Interpolation
    float delayed = buffer[i1] * (1.0f - frac) + buffer[i2] * frac;

    // Allpass canonical form: y[n] = g*x[n] + x[n-m] - g*y[n-m]
    // Which is y = g*x + delayed; buffer[w] = x - g*y;
    float y = g * x + delayed;
    buffer[writeIndex] = x - g * y;

    writeIndex = (writeIndex + 1) % buffer.size();
    return y;
  }
};

class DCBlocker {
private:
  float x_z1 = 0.0f;
  float y_z1 = 0.0f;
  float p = 0.992f;

public:
  float process(float x) {
    float y = x - x_z1 + p * y_z1;
    x_z1 = x;
    y_z1 = y;
    return y;
  }
};

} // namespace paisa
