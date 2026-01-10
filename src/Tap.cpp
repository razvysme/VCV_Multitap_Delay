#include "Tap.hpp"
#include "Multitap_delay.hpp"

namespace paisa {

Tap::Tap(size_t bufferSize) {
  delay = std::unique_ptr<DelayProcessor>(new DelayProcessor(bufferSize));
  filter = std::unique_ptr<FilterProcessor>(new FilterProcessor());
  amppan = std::unique_ptr<AmpPanProcessor>(new AmpPanProcessor());
  fx1 = std::unique_ptr<FX1Processor>(new FX1Processor());
  fx2 = std::unique_ptr<FX2Processor>(new FX2Processor());
}

void Tap::setParam(int mode, float p1, float p2) {
  switch (mode) {
  case Multitap_delay::MODE_DELAY:
    delay->setParams(p1, p2);
    break;
  case Multitap_delay::MODE_AMP_PAN:
    amppan->setParams(p1, p2);
    break;
  case Multitap_delay::MODE_FILTER:
    filter->setParams(p1, p2);
    break;
  case Multitap_delay::MODE_FX1:
    fx1->setParams(p1, p2);
    break;
  case Multitap_delay::MODE_FX2:
    fx2->setParams(p1, p2);
    break;
  }
}

void Tap::process(float inL, float inR, float &outL, float &outR,
                  float sampleRate) {
  // 1. Read from independent delay line
  float tapL, tapR;
  delay->read(tapL, tapR, sampleRate);

  // 2. Head through the processing chain (Filter -> AmpPan -> FX)
  outL = tapL;
  outR = tapR;

  filter->process(outL, outR, sampleRate);
  amppan->process(outL, outR, sampleRate);
  fx1->process(outL, outR, sampleRate);
  fx2->process(outL, outR, sampleRate);

  // 3. Feedback: The end of the chain is fed back to the delay input
  float feedbackGain = delay->getFeedbackAmount();

  // Mix input with feedback
  float fbL = inL + outL * feedbackGain;
  float fbR = inR + outR * feedbackGain;

  // 4. Write back to the independent delay line
  delay->write(fbL, fbR);
}

} // namespace paisa
