#pragma once
#include <cmath>
#define PI 3.14159265358979323846f

namespace paisa {

/**
 * Panner class implementing a constant power panning law.
 * Angle range: 0 (Full Left) to PI/2 (Full Right).
 */
class Panner {

public:
  /**
   * Processes input signal and applies panning.
   * @param pan Normalized pan value from -1.0 (Left) to 1.0 (Right).
   */
  void process(float &left, float &right, float pan) {
    // Normalize pan from [-1, 1] to [0, 1]
    float normalizedPan = (pan + 1.0f) * 0.5f;

    // Map to angle [0, PI/2]
    float angle = normalizedPan * (PI * 0.5f);

    // Constant power panning law:
    // L = cos(angle)
    // R = sin(angle)
    float pL = std::cos(angle);
    float pR = std::sin(angle);

    // Apply panning to input signals
    // Assuming the input might be mono or already stereo, we treat the signals
    // as a pair. If it's a mono source, 'left' and 'right' should be identical
    // before calling this.
    left *= pL;
    right *= pR;
  }
};

} // namespace paisa
