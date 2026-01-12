#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace paisa {

class FDNReverb {
private:
  static constexpr int N4 = 4;
  static constexpr int N8 = 8;

  float delays4[N4] = {1499, 1889, 2381, 2999};
  float delays8[N8] = {809, 877, 937, 1049, 1151, 1249, 1373, 1499};

  float matrixA4[N4 * N4];
  float matrixA8[N8 * N8];

  std::vector<float> buffer4[N4];
  std::vector<float> buffer8[N8];
  int writeIndex4[N4] = {0};
  int writeIndex8[N8] = {0};

  float targetT60 = 1.0f;
  float targetDensity = 0.5f;
  float targetMix = 0.3f;

  float currentT60 = 1.0f;
  float currentDensity = 0.5f;
  float currentMix = 0.3f;

  const float slewing = 0.001f;

  void computeMatrixExp(const float *W_tr, int N, float *A) {
    // Construct skew-symmetric matrix S from W_tr
    std::vector<float> S(N * N, 0.0f);
    int idx = 0;
    for (int i = 0; i < N; ++i) {
      for (int j = i + 1; j < N; ++j) {
        S[i * N + j] = W_tr[idx];
        S[j * N + i] = -W_tr[idx];
        idx++;
      }
    }

    // Taylor series for e^S: I + S + S^2/2! + S^3/3! + ...
    // We'll use 12 terms which is usually plenty for these small values
    std::vector<float> result(N * N, 0.0f);
    std::vector<float> term(N * N, 0.0f);
    std::vector<float> nextTerm(N * N, 0.0f);

    // Result = I
    for (int i = 0; i < N; ++i)
      result[i * N + i] = 1.0f;
    // Term = I
    for (int i = 0; i < N; ++i)
      term[i * N + i] = 1.0f;

    for (int k = 1; k <= 14; ++k) {
      // nextTerm = (term * S) / k
      for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
          float sum = 0.0f;
          for (int l = 0; l < N; ++l) {
            sum += term[i * N + l] * S[l * N + j];
          }
          nextTerm[i * N + j] = sum / (float)k;
        }
      }
      // result += nextTerm
      // term = nextTerm
      for (int i = 0; i < N * N; ++i) {
        result[i] += nextTerm[i];
        term[i] = nextTerm[i];
      }
    }

    // Copy to A
    for (int i = 0; i < N * N; ++i)
      A[i] = result[i];
  }

public:
  FDNReverb() {
    const float W_tr_4[6] = {-0.5182f, 0.2144f,  0.1097f,
                             0.3421f,  -0.1985f, 0.4210f};
    const float W_tr_8[28] = {
        0.1245f,  -0.3210f, 0.0541f,  0.1892f,  -0.0123f, 0.2104f,  -0.0981f,
        0.4321f,  -0.1120f, 0.0876f,  0.3129f,  -0.2451f, 0.0154f,  0.1982f,
        -0.4012f, 0.0651f,  0.1239f,  -0.1872f, 0.2871f,  -0.1092f, 0.3341f,
        0.0452f,  0.0912f,  -0.2210f, 0.1763f,  0.3101f,  -0.0542f, 0.1987f};

    computeMatrixExp(W_tr_4, N4, matrixA4);
    computeMatrixExp(W_tr_8, N8, matrixA8);

    for (int i = 0; i < N4; ++i) {
      buffer4[i].resize((int)delays4[i] + 1, 0.0f);
    }
    for (int i = 0; i < N8; ++i) {
      buffer8[i].resize((int)delays8[i] + 1, 0.0f);
    }
  }

  void setParams(float mixVal, float decay, float density) {
    targetMix = mixVal;
    // Map decay (0-1) to T60 (e.g., 0.1s to 10s)
    targetT60 = 0.1f + std::pow(decay, 2.0f) * 9.9f;
    targetDensity = density;
  }

  void process(float &left, float &right, float sampleRate) {
    float dryL = left;
    float dryR = right;
    float in = (left + right) * 0.5f;

    if (sampleRate < 1.0f)
      return;

    currentT60 += (targetT60 - currentT60) * slewing;
    currentDensity += (targetDensity - currentDensity) * slewing;
    currentMix += (targetMix - currentMix) * slewing;

    if (!std::isfinite(currentMix))
      currentMix = 0.0f;

    float gamma = std::pow(10.0f, -3.0f / (sampleRate * currentT60));

    // N=4 Engine
    float delayed4[N4];
    for (int i = 0; i < N4; ++i) {
      delayed4[i] = buffer4[i][writeIndex4[i]];
    }

    float v4[N4];
    for (int i = 0; i < N4; ++i) {
      float sum = 0.0f;
      for (int j = 0; j < N4; ++j) {
        sum += matrixA4[i * N4 + j] * delayed4[j];
      }
      v4[i] = sum;
    }

    // b = [1, 1, 1, 1], c = [1/4, 1/4, 1/4, 1/4]
    float out4 = 0.0f;
    for (int i = 0; i < N4; ++i) {
      // Homogeneous decay gain gamma applied per line relative to its length
      // wait, if gamma is per-sample, gain is gamma^delay
      float gi = std::pow(gamma, delays4[i]);
      buffer4[i][writeIndex4[i]] = in + v4[i] * gi;
      writeIndex4[i] = (writeIndex4[i] + 1) % buffer4[i].size();
      out4 += delayed4[i] * 0.25f;
    }

    // N=8 Engine
    float delayed8[N8];
    for (int i = 0; i < N8; ++i) {
      delayed8[i] = buffer8[i][writeIndex8[i]];
    }

    float v8[N8];
    for (int i = 0; i < N8; ++i) {
      float sum = 0.0f;
      for (int j = 0; j < N8; ++j) {
        sum += matrixA8[i * N8 + j] * delayed8[j];
      }
      v8[i] = sum;
    }

    float out8 = 0.0f;
    for (int i = 0; i < N8; ++i) {
      float gi = std::pow(gamma, delays8[i]);
      buffer8[i][writeIndex8[i]] = in + v8[i] * gi;
      writeIndex8[i] = (writeIndex8[i] + 1) % buffer8[i].size();
      out8 += delayed8[i] * 0.125f;
    }

    // Power-constant crossfade between N=4 and N=8 based on Density
    // Density 0 -> N=4, Density 1 -> N=8
    float angle = currentDensity * M_PI * 0.5f;
    float w4 = std::cos(angle);
    float w8 = std::sin(angle);
    float wet = out4 * w4 + out8 * w8;

    left = dryL + currentMix * wet;
    right = dryR + currentMix * wet;

    if (!std::isfinite(left))
      left = dryL;
    if (!std::isfinite(right))
      right = dryR;
  }
};

} // namespace paisa
