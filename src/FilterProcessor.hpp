#pragma once
#include "Filter.hpp"
#include "Processor.hpp"

namespace paisa {

class FilterProcessor : public Processor {
  BaseWidthFilter filterL;
  BaseWidthFilter filterR;
  float baseFreq = 1000.0f;
  float width = 50.0f;
  float offsetBase = 0.0f;
  float offsetWidth = 0.0f;

public:
  void setParams(float p1, float p2) override {
    baseFreq =
        std::exp(std::log(20.f) + p1 * (std::log(20000.f) - std::log(20.f)));
    width = p2 * 100.f;
  }

  void setOffsets(float o1, float o2) override {
    offsetBase = o1 * 10000.f;
    offsetWidth = o2 * 50.f;
  }

  void process(float &left, float &right, float sampleRate) override {
    float f = std::max(20.0f, std::min(20000.0f, baseFreq + offsetBase));
    float w = std::max(0.0f, std::min(100.0f, width + offsetWidth));

    filterL.setParams(sampleRate, f, w);
    filterR.setParams(sampleRate, f, w);

    left = filterL.process(left);
    right = filterR.process(right);
  }
};

} // namespace paisa
