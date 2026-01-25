#include "Filter.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Filter::Filter(int sampleRate) : sampleRate_(static_cast<float>(sampleRate)) {
  states_.resize(2); // Stereo
  reset();
}

void Filter::reset() {
  for (auto &state : states_) {
    state.lp = 0.0f;
    state.bp = 0.0f;
    state.hp = 0.0f;
  }
}

void Filter::setParameters(float x, float y) {
  // Cutoff: Exponential mapping 20Hz - 20kHz
  // x = 0 -> 20Hz
  // x = 1 -> 20000Hz
  // 20 * (1000)^x
  targetCutoff_ = 20.0f * std::pow(1000.0f, x);
  targetCutoff_ = std::min(targetCutoff_,
                           sampleRate_ / 2.0f - 100.0f); // Clamp below Nyquist

  // Resonance: 0.0 - 0.95 (safety cap)
  targetResonance_ = y * 0.95f;
}

void Filter::process(float *buffer, int frames, int channels) {
  // Simple 1-pole smoothing coefficient
  const float smooth = 0.001f;

  for (int i = 0; i < frames; ++i) {
    // Parameter smoothing
    cutoff_ += (targetCutoff_ - cutoff_) * smooth;
    resonance_ += (targetResonance_ - resonance_) * smooth;

    // Calculate filter coefficients (Chamberlin SVF)
    // f = 2 * sin(pi * cutoff / samplerate)
    // To keep it stable, f should be < 1.0 (approx < fs/6)
    // For higher freqs, we might need oversampling or a different topology,
    // but for a basic effect this is often sufficient if clamped.
    float f = 2.0f * std::sin(M_PI * cutoff_ / sampleRate_);
    f = std::min(f, 1.0f); // Stability clamp

    float q = 1.0f - resonance_;

    for (int c = 0; c < channels; ++c) {
      ChannelState &s = states_[c];
      float input = buffer[i * channels + c];

      // SVF Process
      s.hp = input - s.lp - q * s.bp;
      s.bp += f * s.hp;
      s.lp += f * s.bp;

      // Output Low Pass
      buffer[i * channels + c] = s.lp;
    }
  }
}
