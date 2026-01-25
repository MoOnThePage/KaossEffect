#ifndef KAOSSEFFECT_FILTER_H
#define KAOSSEFFECT_FILTER_H

#include "../utils/SmoothValue.h"
#include "Effect.h"
#include <vector>

class Filter : public Effect {
public:
  Filter(int sampleRate);

  void setParameters(float x, float y) override;
  void process(float *buffer, int frames, int channels) override;
  void reset() override;

private:
  float sampleRate_;

  // Target parameters
  float targetCutoff_ = 1000.0f;
  float targetResonance_ = 0.0f;

  // Smoothed parameters
  float cutoff_ = 1000.0f;
  float resonance_ = 0.0f;

  // Filter state (stereo)
  struct ChannelState {
    float lp = 0.0f;
    float bp = 0.0f;
    float hp = 0.0f;
  };

  std::vector<ChannelState> states_;
};

#endif // KAOSSEFFECT_FILTER_H
