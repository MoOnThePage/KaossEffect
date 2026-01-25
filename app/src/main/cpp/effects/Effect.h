#ifndef KAOSSEFFECT_EFFECT_H
#define KAOSSEFFECT_EFFECT_H

class Effect {
public:
  virtual ~Effect() = default;

  // x and y are normalized 0.0 - 1.0 parameters
  virtual void setParameters(float x, float y) = 0;

  // Process audio buffer in-place
  // buffer: interleaved stereo float samples
  // frames: number of frames (pairs of samples)
  // channels: number of channels (usually 2)
  virtual void process(float *buffer, int frames, int channels) = 0;

  virtual void reset() = 0;
};

#endif // KAOSSEFFECT_EFFECT_H
