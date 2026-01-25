#include "Delay.h"
#include <algorithm>
#include <cmath>

Delay::Delay(int sampleRate, int maxDelayMs)
    : sampleRate_(static_cast<float>(sampleRate)) {
  bufferSize_ = (sampleRate * maxDelayMs) / 1000;
  // Ensure power of 2 for cheap wrapping if desired, but modulo is fine for now
  // Actually simple modulo is fine.

  for (int i = 0; i < 2; ++i) {
    delayBuffer_[i].resize(bufferSize_, 0.0f);
  }
  reset();
}

void Delay::reset() {
  for (int i = 0; i < 2; ++i) {
    std::fill(delayBuffer_[i].begin(), delayBuffer_[i].end(), 0.0f);
  }
  writePos_ = 0;
}

void Delay::setParameters(float x, float y) {
  // X -> Delay Time (10ms to 500ms)
  // 0.01s - 0.5s
  targetDelayTime_ = 0.01f + (0.49f * x);

  // Y -> Feedback (0.0 to 0.9)
  targetFeedback_ = y * 0.9f;
}

void Delay::process(float *buffer, int frames, int channels) {
  const float smooth = 0.001f;
  const float mix = 0.5f; // Fixed 50% wet/dry for now (or make it vary?)

  for (int i = 0; i < frames; ++i) {
    // Parameter smoothing
    delayTime_ += (targetDelayTime_ - delayTime_) * smooth;
    feedback_ += (targetFeedback_ - feedback_) * smooth;

    float delaySamples = delayTime_ * sampleRate_;

    for (int c = 0; c < channels; ++c) {
      if (c >= 2)
        break; // Support max stereo

      // Read pointer
      float readPos = static_cast<float>(writePos_) - delaySamples;
      while (readPos < 0.0f)
        readPos += bufferSize_;
      while (readPos >= bufferSize_)
        readPos -= bufferSize_;

      // Linear Interpolation
      int idxA = static_cast<int>(readPos);
      int idxB = (idxA + 1) % bufferSize_;
      float frac = readPos - idxA;

      float delayedSample =
          delayBuffer_[c][idxA] * (1.0f - frac) + delayBuffer_[c][idxB] * frac;

      float input = buffer[i * channels + c];
      float output = input + delayedSample * mix;

      // Write to buffer with feedback
      // Soft clip feedback to prevent explosion?
      float feedbackSample = input + delayedSample * feedback_;
      delayBuffer_[c][writePos_] = std::clamp(feedbackSample, -2.0f, 2.0f);

      buffer[i * channels + c] = output;
    }

    writePos_++;
    if (writePos_ >= bufferSize_)
      writePos_ = 0;
  }
}
