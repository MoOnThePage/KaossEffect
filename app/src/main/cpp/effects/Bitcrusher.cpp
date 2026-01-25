#include "Bitcrusher.h"
#include <algorithm>
#include <cmath>

Bitcrusher::Bitcrusher(int sampleRate)
    : sampleRate_(static_cast<float>(sampleRate)) {
  heldSample_.resize(2, 0.0f); // Stereo
  reset();
}

void Bitcrusher::reset() {
  std::fill(heldSample_.begin(), heldSample_.end(), 0.0f);
  holdCounter_ = 0;
}

void Bitcrusher::setParameters(float x, float y) {
  // X -> Bit Depth (16 downto 2)
  // Invert X so moving right increases effect (lower bits)
  // x=0 -> 16 bits, x=1 -> 2 bits
  targetBits_ = 16.0f - (x * 14.0f);

  // Y -> Rate Reduction (1 to 32)
  // y=0 -> 1 (normal), y=1 -> 32 (extreme aliasing)
  targetRateDiv_ = 1.0f + (y * 31.0f);
}

void Bitcrusher::process(float *buffer, int frames, int channels) {
  const float smooth = 0.005f;

  for (int i = 0; i < frames; ++i) {
    // Parameter smoothing
    bits_ += (targetBits_ - bits_) * smooth;
    rateDiv_ += (targetRateDiv_ - rateDiv_) * smooth;

    int currentDiv = static_cast<int>(rateDiv_);
    if (currentDiv < 1)
      currentDiv = 1;

    // Quantization steps = 2^bits
    float steps = std::pow(2.0f, bits_);

    // Rate Reduction Logic
    bool captureNew = (holdCounter_ == 0);

    for (int c = 0; c < channels; ++c) {
      float sample;

      if (captureNew) {
        // Bit Crush
        float input = buffer[i * channels + c];

        // Quantize
        // steps/2 because audio is signed -1 to 1?
        // Actually usually we map -1..1 to 0..steps or just multiply.
        // floor(sample * steps) / steps

        // A primitive bitcrush:
        sample = std::floor(input * steps) / steps;

        // Store for holding
        heldSample_[c] = sample;
      } else {
        // Output held sample
        sample = heldSample_[c];
      }

      buffer[i * channels + c] = sample;
    }

    // Advance counter
    holdCounter_++;
    if (holdCounter_ >= currentDiv) {
      holdCounter_ = 0;
    }
  }
}
