#include "AudioEngine.h"
#include <algorithm>
#include <android/log.h>
#include <cmath>

#define TAG "AudioEngine"

AudioEngine *AudioEngine::getInstance() {
  static AudioEngine engine;
  return &engine;
}

AudioEngine::~AudioEngine() { stopStream(); }

AudioEngine::AudioEngine() {
  filter_ = std::make_unique<Filter>(kSampleRate);
  chorus_ = std::make_unique<Chorus>(kSampleRate);
  reverb_ = std::make_unique<Reverb>(kSampleRate);
  phaser_ = std::make_unique<Phaser>(kSampleRate);
  bitcrusher_ = std::make_unique<Bitcrusher>(kSampleRate);
  bitcrusher_ = std::make_unique<Bitcrusher>(kSampleRate);
  ringMod_ = std::make_unique<RingMod>(kSampleRate);
  visualizerBuffer_.resize(kVisualizerBufferSize, 0.0f);
}

// Lifecycle methods
void AudioEngine::startStream() {
  oboe::AudioStreamBuilder builder;
  builder.setDirection(oboe::Direction::Output);
  builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
  builder.setSharingMode(oboe::SharingMode::Shared);
  builder.setFormat(oboe::AudioFormat::Float);
  builder.setChannelCount(kChannelCount);
  builder.setSampleRate(kSampleRate);
  builder.setCallback(this);

  oboe::Result result = builder.openStream(stream_);
  if (result != oboe::Result::OK) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to open stream: %s",
                        oboe::convertToText(result));
    return;
  }

  result = stream_->requestStart();
  if (result != oboe::Result::OK) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to start stream: %s",
                        oboe::convertToText(result));
    stream_->close();
    return;
  }

  const int32_t capacityFrames = stream_->getBufferCapacityInFrames();
  if (capacityFrames > 0) {
    mixBuffer_.resize(capacityFrames * kChannelCount, 0.0f);
  }

  __android_log_print(ANDROID_LOG_INFO, TAG, "Stream started");
}

void AudioEngine::stopStream() {
  if (stream_) {
    stream_->stop();
    stream_->close();
    stream_.reset();
  }
  __android_log_print(ANDROID_LOG_INFO, TAG, "Stream stopped");
}

void AudioEngine::setXY(int slot, float x, float y) {
  if (slot == 0) {
    paramAX_.store(x);
    paramAY_.store(y);
  } else {
    paramBX_.store(x);
    paramBY_.store(y);
  }
}

void AudioEngine::setEffectMode(int slot, int mode) {
  if (slot == 0) {
    effectModeA_.store(mode);
  } else {
    effectModeB_.store(mode);
  }
}

void AudioEngine::setWetMix(int slot, float mix) {
  float clamped = std::min(1.0f, std::max(0.0f, mix));
  if (slot == 0) {
    wetMixA_.store(clamped);
  } else {
    wetMixB_.store(clamped);
  }
}

bool AudioEngine::isPlaying() { return isPlaying_.load(); }

bool AudioEngine::loadFile(int fd, int64_t offset, int64_t size) {
  std::lock_guard<std::mutex> lock(decoderMutex_);
  bool wasPlaying = isPlaying_.load();
  isPlaying_.store(false);

  auto newDecoder = std::make_unique<Mp3Decoder>();
  if (newDecoder->open(fd, offset, size)) {
    decoder_ = std::move(newDecoder);
    __android_log_print(ANDROID_LOG_INFO, TAG,
                        "File loaded successfully. Duration: %lld",
                        decoder_->getDurationMs());
    return true;
  }
  __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to open mp3 decoder");
  return false;
}

void AudioEngine::play() {
  isPlaying_.store(true);
  __android_log_print(ANDROID_LOG_INFO, TAG,
                      "Play requested. isPlaying = true");
}

void AudioEngine::pause() {
  isPlaying_.store(false);
  __android_log_print(ANDROID_LOG_INFO, TAG,
                      "Pause requested. isPlaying = false");
}

// Transport Stop: Stop playback and rewind
void AudioEngine::stop() {
  isPlaying_.store(false);
  seekTo(0);
  __android_log_print(ANDROID_LOG_INFO, TAG, "Stop requested.");
}

void AudioEngine::seekTo(int64_t positionMs) {
  std::lock_guard<std::mutex> lock(decoderMutex_);
  if (decoder_) {
    decoder_->seekTo(positionMs);
  }
}

int64_t AudioEngine::getDurationMs() {
  std::lock_guard<std::mutex> lock(decoderMutex_);
  if (decoder_) {
    return decoder_->getDurationMs();
  }
  return 0;
}

int64_t AudioEngine::getPositionMs() {
  // No lock needed for simple atomic read if we cached it,
  // but decoder access needs lock or atomic wrapper.
  // For now, lock to be safe.
  std::lock_guard<std::mutex> lock(decoderMutex_);
  if (decoder_) {
    return decoder_->getPositionMs();
  }
  return 0;
}

int AudioEngine::getVisualizerData(float *buffer, int size) {
  std::lock_guard<std::mutex> lock(visualizerMutex_);
  int validCount = visualizerSampleCount_.load();
  int copySize =
      std::min(size, std::min(validCount, (int)visualizerBuffer_.size()));

  if (copySize > 0) {
    std::copy(visualizerBuffer_.begin(), visualizerBuffer_.begin() + copySize,
              buffer);
  }
  return copySize;
}

oboe::DataCallbackResult
AudioEngine::onAudioReady(oboe::AudioStream *audioStream, void *audioData,
                          int32_t numFrames) {
  auto *outputData = static_cast<float *>(audioData);

  // Default to silence
  std::fill(outputData, outputData + numFrames * kChannelCount, 0.0f);

  if (!isPlaying_.load())
    return oboe::DataCallbackResult::Continue;

  std::unique_lock<std::mutex> lock(decoderMutex_, std::try_to_lock);
  if (!lock.owns_lock() || !decoder_) {
    // Output silence if locked (loading file) or no decoder
    return oboe::DataCallbackResult::Continue;
  }

  int samplesRead = decoder_->read(outputData, numFrames * kChannelCount);

  // Debug logging for starvation (throttled)
  static int starvationLogCounter = 0;
  if (samplesRead == 0 && !decoder_->isEndOfStream()) {
    starvationLogCounter++;
    if (starvationLogCounter % 100 ==
        1) { // Log every ~1s (assuming 10ms-ish callback)
      __android_log_print(ANDROID_LOG_WARN, TAG,
                          "Starvation detected: read 0 samples but not EOS");
    }
  } else {
    starvationLogCounter = 0;
  }

  if (samplesRead < numFrames * kChannelCount) {
    if (decoder_->isEndOfStream()) {
      isPlaying_.store(false);
      __android_log_print(ANDROID_LOG_INFO, TAG, "Playback finished (EOS)");
    }
  }

  auto processEffectMode = [&](int mode, float x, float y, int frames) {
    if (mode == 0 && filter_) { // Filter
      filter_->setParameters(x, y);
      filter_->process(outputData, frames, kChannelCount);
    } else if (mode == 1 && chorus_) { // Chorus
      chorus_->setParameters(x, y);
      chorus_->process(outputData, frames, kChannelCount);
    } else if (mode == 2 && reverb_) { // Reverb
      reverb_->setParameters(x, y);
      reverb_->process(outputData, frames, kChannelCount);
    } else if (mode == 3 && phaser_) { // Phaser
      phaser_->setParameters(x, y);
      phaser_->process(outputData, frames, kChannelCount);
    } else if (mode == 4 && bitcrusher_) { // Bitcrusher
      bitcrusher_->setParameters(x, y);
      bitcrusher_->process(outputData, frames, kChannelCount);
    } else if (mode == 5 && ringMod_) { // RingMod
      ringMod_->setParameters(x, y);
      ringMod_->process(outputData, frames, kChannelCount);
    }
  };

  const int framesRead = samplesRead / kChannelCount;
  if (framesRead > 0) {
    const int sampleCount = framesRead * kChannelCount;

    const int modeA = effectModeA_.load();
    const float wetA = wetMixA_.load();
    const float ax = paramAX_.load();
    const float ay = paramAY_.load();

    if (modeA >= 0 && wetA > 0.0f) {
      if (wetA < 1.0f) {
        if ((int)mixBuffer_.size() < sampleCount) {
          mixBuffer_.resize(sampleCount, 0.0f);
        }
        std::copy(outputData, outputData + sampleCount, mixBuffer_.begin());
      }
      processEffectMode(modeA, ax, ay, framesRead);
      if (wetA < 1.0f) {
        for (int i = 0; i < sampleCount; ++i) {
          const float dry = mixBuffer_[i];
          outputData[i] = dry + (outputData[i] - dry) * wetA;
        }
      }
    }

    const int modeB = effectModeB_.load();
    const float wetB = wetMixB_.load();
    const float bx = paramBX_.load();
    const float by = paramBY_.load();

    if (modeB >= 0 && wetB > 0.0f) {
      if (wetB < 1.0f) {
        if ((int)mixBuffer_.size() < sampleCount) {
          mixBuffer_.resize(sampleCount, 0.0f);
        }
        std::copy(outputData, outputData + sampleCount, mixBuffer_.begin());
      }
      processEffectMode(modeB, bx, by, framesRead);
      if (wetB < 1.0f) {
        for (int i = 0; i < sampleCount; ++i) {
          const float dry = mixBuffer_[i];
          outputData[i] = dry + (outputData[i] - dry) * wetB;
        }
      }
    }
  }

  // Soft Clipper (tanh-like limiting) to prevent harsh digital clipping
  // simple polynomial: f(x) = x - x^3/3 for x in [-1.5, 1.5] roughly covers it,
  // or just hard clamp after some saturation.
  // We'll use a fast saturation curve: x / (1 + |x|) is simple but effective.
  // Or std::tanh which sounds nice.
  for (int i = 0; i < numFrames * kChannelCount; ++i) {
    float x = outputData[i];
    // Fast sigmoid: x / (1 + |x|) approaches +/- 1.0 asymptotically
    // Let's use std::tanh for "warm" overdrive behavior
    // outputData[i] = std::tanh(x);

    // Actually, let's use a slightly harder knee to preserve volume until it
    // hits the limit. If x > 1.0, clamp. But we want to avoid hard clip. Let's
    // stick to std::clamp for safety first, effectively hard limiting, but
    // maybe a little soft knee? Let's use simple hard clamping for now to
    // strictly satisfy "prevent harsh clipping" (which usually means
    // wrapping/overflow). But user asked for "soft clipper". Simple soft clip:
    // if (x > 1.0f) x = 1.0f; but that's hard clip. x = x < -1.5f ? -1.0f : (x
    // > 1.5f ? 1.0f : ...);

    // Standard cubic soft clipper
    if (x < -1.5f) {
      x = -1.0f;
    } else if (x > 1.5f) {
      x = 1.0f;
    } else {
      x = x - (x * x * x) / 27.0f; // Soft saturation up to 1.5 input
      // wait, derivative at 1.5 is 1 - 3*1.5^2/27 = 1 - 6.75/27 = 0.75?
      // The formula x - x^3/3 is standard for [-1, 1].
    }

    // Let's just use std::tanh, it's reliable and sounds good.
    outputData[i] = std::tanh(x);
  }

  // Capture for visualizer
  if (numFrames > 0) {
    std::lock_guard<std::mutex> lock(visualizerMutex_);
    int captureSize = std::min((int)numFrames, kVisualizerBufferSize);

    // Simple downsampling/copying: take every Nth sample or just first N
    // Taking first N samples (Mono mix) is simplest and sufficient for waveform
    for (int i = 0; i < captureSize; ++i) {
      // Average L+R for mono
      float left = outputData[i * kChannelCount];
      float right = outputData[i * kChannelCount + 1];
      visualizerBuffer_[i] = (left + right) * 0.5f;
    }
    visualizerSampleCount_.store(captureSize);
  }

  return oboe::DataCallbackResult::Continue;
}
