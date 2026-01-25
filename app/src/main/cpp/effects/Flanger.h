#ifndef KAOSSEFFECT_FLANGER_H
#define KAOSSEFFECT_FLANGER_H

#include "../utils/SmoothValue.h"
#include "Effect.h"
#include <vector>

class Flanger : public Effect {
public:
  Flanger(int sampleRate);

  void setParameters(float x, float y) override;
  void process(float *buffer, int frames, int channels) override;
  void reset() override;

private:
  float sampleRate_;

  // Delay lines (short)
  std::vector<float> delayBuffer_[2];
  int bufferSize_ = 0;
  int writePos_ = 0;

  // LFO State
  float lfoPhase_ = 0.0f;

  // Target parameters
  float targetRate_ = 0.5f;
  float targetDepth_ = 0.5f;

  // Smoothed parameters
  float lfoRate_ = 0.5f;
  float depth_ = 0.5f;    // 0.0 - 1.0 (maps to 0 - 10ms excursion)
  float feedback_ = 0.6f; // Fixed feedback for now, or could map to something
};

#endif // KAOSSEFFECT_FLANGER_H
