#include "Flanger.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Flanger::Flanger(int sampleRate) : sampleRate_(static_cast<float>(sampleRate)) {
  // 50ms buffer is plenty for flanger
  bufferSize_ = (sampleRate * 50) / 1000;

  for (int i = 0; i < 2; ++i) {
    delayBuffer_[i].resize(bufferSize_, 0.0f);
  }
  reset();
}

void Flanger::reset() {
  for (int i = 0; i < 2; ++i) {
    std::fill(delayBuffer_[i].begin(), delayBuffer_[i].end(), 0.0f);
  }
  writePos_ = 0;
  lfoPhase_ = 0.0f;
}

void Flanger::setParameters(float x, float y) {
  // X -> LFO Rate (0.05Hz to 5Hz)
  // 0.05 * (100^x)
  targetRate_ = 0.05f * std::pow(100.0f, x);

  // Y -> Depth (0.0 to 1.0)
  // 1.0 means full 10ms sweep
  targetDepth_ = y;
}

void Flanger::process(float *buffer, int frames, int channels) {
  const float smooth = 0.001f;
  const float baseDelayMs = 1.0f;   // 1ms fixed base delay
  const float sweepWidthMs = 10.0f; // 10ms max sweep

  for (int i = 0; i < frames; ++i) {
    // Parameter smoothing
    lfoRate_ += (targetRate_ - lfoRate_) * smooth;
    depth_ += (targetDepth_ - depth_) * smooth;

    // LFO Update
    // lfoPhase goes 0.0 -> 1.0
    lfoPhase_ += lfoRate_ / sampleRate_;
    if (lfoPhase_ >= 1.0f)
      lfoPhase_ -= 1.0f;

    // Calculate LFO value (0.0 to 1.0 sine)
    float lfo = 0.5f + 0.5f * std::sin(2.0f * M_PI * lfoPhase_);

    // Modulated Delay Time
    float currentDelayMs = baseDelayMs + (lfo * depth_ * sweepWidthMs);
    float delaySamples = currentDelayMs * sampleRate_ / 1000.0f;

    for (int c = 0; c < channels; ++c) {
      if (c >= 2)
        break;

      // Read pointer
      float readPos = static_cast<float>(writePos_) - delaySamples;
      while (readPos < 0.0f)
        readPos += bufferSize_;
      while (readPos >= bufferSize_)
        readPos -= bufferSize_;

      // Interpolation
      int idxA = static_cast<int>(readPos);
      int idxB = (idxA + 1) % bufferSize_;
      float frac = readPos - idxA;

      float delayed =
          delayBuffer_[c][idxA] * (1.0f - frac) + delayBuffer_[c][idxB] * frac;

      float input = buffer[i * channels + c];

      // Output = Input + Delayed (Comb Filtering)
      // Mix 50/50
      buffer[i * channels + c] = (input + delayed) * 0.5f;

      // Feedback
      float feedbackSample = input + delayed * feedback_;
      delayBuffer_[c][writePos_] = std::clamp(feedbackSample, -2.0f, 2.0f);
    }

    writePos_++;
    if (writePos_ >= bufferSize_)
      writePos_ = 0;
  }
}
