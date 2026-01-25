#ifndef KAOSSEFFECT_DELAY_H
#define KAOSSEFFECT_DELAY_H

#include "../utils/SmoothValue.h"
#include "Effect.h"
#include <vector>

class Delay : public Effect {
public:
  Delay(int sampleRate, int maxDelayMs = 1000);

  void setParameters(float x, float y) override;
  void process(float *buffer, int frames, int channels) override;
  void reset() override;

private:
  float sampleRate_;

  // Stereo delay lines
  std::vector<float> delayBuffer_[2];
  int bufferSize_ = 0;
  int writePos_ = 0;

  // Parameters
  float targetDelayTime_ = 0.5f; // Seconds
  float targetFeedback_ = 0.0f;

  // Smoothed
  float delayTime_ = 0.5f;
  float feedback_ = 0.0f;

  // Internal
  float currentDelaySamples_ = 0.0f;
};

#endif // KAOSSEFFECT_DELAY_H
