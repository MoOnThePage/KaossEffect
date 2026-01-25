#ifndef KAOSSEFFECT_BITCRUSHER_H
#define KAOSSEFFECT_BITCRUSHER_H

#include "../utils/SmoothValue.h"
#include "Effect.h"
#include <vector>

class Bitcrusher : public Effect {
public:
  Bitcrusher(int sampleRate);

  void setParameters(float x, float y) override;
  void process(float *buffer, int frames, int channels) override;
  void reset() override;

private:
  float sampleRate_;

  // Target parameters
  float targetBits_ = 16.0f;
  float targetRateDiv_ = 1.0f;

  // Smoothed parameters
  float bits_ = 16.0f;
  float rateDiv_ = 1.0f;

  // Rate reduction state
  std::vector<float> heldSample_;
  int holdCounter_ = 0;
};

#endif // KAOSSEFFECT_BITCRUSHER_H
