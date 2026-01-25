#ifndef KAOSSEFFECT_CHORUS_H
#define KAOSSEFFECT_CHORUS_H

#include <vector>

class Chorus {
public:
  Chorus(int sampleRate);
  void process(float *buffer, int frames, int channels);
  void setParameters(float x, float y);
  void reset();

private:
  float sampleRate_;
  std::vector<float> delayBuffer_[2];
  int writePos_ = 0;
  int bufferSize_ = 0;

  // LFO
  float lfoPhase_ = 0.0f;
  float lfoRate_ = 1.0f;
  float depth_ = 0.0f;

  // Targets for smoothing
  float targetRate_ = 1.0f;
  float targetDepth_ = 0.0f;
};

#endif // KAOSSEFFECT_CHORUS_H
