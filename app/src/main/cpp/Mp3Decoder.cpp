#include "Mp3Decoder.h"
#include <android/log.h>
#include <cmath>

#define TAG "Mp3Decoder"

Mp3Decoder::Mp3Decoder() {}

Mp3Decoder::~Mp3Decoder() { close(); }

bool Mp3Decoder::open(int fd, int64_t offset, int64_t size) {
  close(); // Ensure clean state

  extractor_ = AMediaExtractor_new();
  if (!extractor_) {
    __android_log_print(ANDROID_LOG_ERROR, TAG,
                        "Failed to create MediaExtractor");
    return false;
  }

  media_status_t err =
      AMediaExtractor_setDataSourceFd(extractor_, fd, offset, size);
  if (err != AMEDIA_OK) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to set data source: %d",
                        err);
    AMediaExtractor_delete(extractor_);
    extractor_ = nullptr;
    return false;
  }

  size_t numTracks = AMediaExtractor_getTrackCount(extractor_);
  int selectedTrack = -1;
  AMediaFormat *format = nullptr;

  for (size_t i = 0; i < numTracks; ++i) {
    format = AMediaExtractor_getTrackFormat(extractor_, i);
    const char *mime;
    if (AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)) {
      if (std::string(mime).find("audio/") == 0) {
        AMediaExtractor_selectTrack(extractor_, i);
        selectedTrack = i;
        break; // Found audio track
      }
    }
    AMediaFormat_delete(format);
    format = nullptr;
  }

  if (selectedTrack < 0 || !format) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "No audio track found");
    if (format)
      AMediaFormat_delete(format);
    // extractor cleanup handled in close() usually, but here we can just close
    return false;
  }

  AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sampleRate_);
  AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channelCount_);
  AMediaFormat_getInt64(format, AMEDIAFORMAT_KEY_DURATION, &durationMs_);
  durationMs_ /= 1000;

  __android_log_print(ANDROID_LOG_INFO, TAG,
                      "Format: %d Hz, %d channels, %lld ms", sampleRate_,
                      channelCount_, durationMs_);

  const char *mime;
  AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime);
  codec_ = AMediaCodec_createDecoderByType(mime);

  if (!codec_) {
    __android_log_print(ANDROID_LOG_ERROR, TAG,
                        "Failed to create decoder for mime: %s", mime);
    AMediaFormat_delete(format);
    return false;
  }

  AMediaCodec_configure(codec_, format, nullptr, nullptr, 0);
  AMediaCodec_start(codec_);

  AMediaFormat_delete(format);

  isEOS_ = false;
  sawInputEOS_ = false;
  sawOutputEOS_ = false;
  decodeBuffer_.reserve(4096); // Basic reservation

  return true;
}

void Mp3Decoder::close() {
  if (codec_) {
    AMediaCodec_stop(codec_);
    AMediaCodec_delete(codec_);
    codec_ = nullptr;
  }
  if (extractor_) {
    AMediaExtractor_delete(extractor_);
    extractor_ = nullptr;
  }
  decodeBuffer_.clear();
  decodeBufferReadPtr_ = 0;
}

int Mp3Decoder::read(float *buffer, int samplesToRead) {
  if (!codec_ || !extractor_)
    return 0;
  if (isEOS_ && decodeBufferReadPtr_ == decodeBuffer_.size())
    return 0;

  int samplesWritten = 0;
  int retryCount = 0;
  const int MAX_RETRIES =
      500; // Allow more spinning to feed codec after flush/seek

  while (samplesWritten < samplesToRead) {
    // 1. Drain internal buffer first
    if (decodeBufferReadPtr_ < decodeBuffer_.size()) {
      int available = decodeBuffer_.size() - decodeBufferReadPtr_;
      int toCopy = std::min(available, samplesToRead - samplesWritten);
      memcpy(buffer + samplesWritten,
             decodeBuffer_.data() + decodeBufferReadPtr_,
             toCopy * sizeof(float));

      samplesWritten += toCopy;
      decodeBufferReadPtr_ += toCopy;

      if (decodeBufferReadPtr_ == decodeBuffer_.size()) {
        decodeBuffer_.clear();
        decodeBufferReadPtr_ = 0;
      }

      // If we filled the request, break immediately
      if (samplesWritten == samplesToRead)
        break;
    }

    if (sawOutputEOS_) {
      isEOS_ = true;
      break;
    }

    // Check break condition: if we tried too many times without result
    if (retryCount > MAX_RETRIES) {
      break;
    }

    // 2. Feed input to codec
    // Use 0 timeout (non-blocking) to avoid holding audio thread
    bool didWork = false;
    if (!sawInputEOS_) {
      ssize_t bufIdx = AMediaCodec_dequeueInputBuffer(codec_, 0);
      if (bufIdx >= 0) {
        size_t bufSize;
        uint8_t *buf = AMediaCodec_getInputBuffer(codec_, bufIdx, &bufSize);

        if (!buf) {
          // Should not happen if bufIdx >= 0, but safety first
          didWork = false;
        } else {
          ssize_t sampleSize =
              AMediaExtractor_readSampleData(extractor_, buf, bufSize);
          if (sampleSize < 0) {
            sampleSize = 0;
            sawInputEOS_ = true;
            AMediaCodec_queueInputBuffer(codec_, bufIdx, 0, 0, 0,
                                         AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
          } else {
            int64_t pts = AMediaExtractor_getSampleTime(extractor_);
            AMediaCodec_queueInputBuffer(codec_, bufIdx, 0, sampleSize, pts, 0);
            AMediaExtractor_advance(extractor_);
          }
          didWork = true;
        }
      }
    }

    // 3. Drain output from codec
    // Loop to drain ALL available output
    while (processOutput()) {
      didWork = true;
    }

    // If internal buffer has data now, the loop will catch it at step 1.
    // If we didn't do any work (no input slot, no output ready), increment
    // retry.
    if (!didWork) {
      retryCount++;
    }
  }

  return samplesWritten;
}

bool Mp3Decoder::processOutput() {
  AMediaCodecBufferInfo info;
  ssize_t bufIdx = AMediaCodec_dequeueOutputBuffer(codec_, &info, 0);

  if (bufIdx >= 0) {
    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
      sawOutputEOS_ = true;
    }

    if (info.size > 0) {
      size_t bufSize;
      uint8_t *buf = AMediaCodec_getOutputBuffer(codec_, bufIdx, &bufSize);

      if (buf) {
        // Assume 16-bit PCM (common for mp3 decoder)
        int16_t *pcmSamples = reinterpret_cast<int16_t *>(buf + info.offset);
        int numSamples = info.size / sizeof(int16_t);

        decodeBuffer_.reserve(decodeBuffer_.size() + numSamples);
        for (int i = 0; i < numSamples; ++i) {
          // Convert S16 to Float
          decodeBuffer_.push_back(pcmSamples[i] / 32768.0f);
        }

        // Update approximate position from PTS
        currentPositionMs_ = info.presentationTimeUs / 1000;
      }
    }

    AMediaCodec_releaseOutputBuffer(codec_, bufIdx, false);
    return true; // We produced data
  } else if (bufIdx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
    auto format = AMediaCodec_getOutputFormat(codec_);
    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sampleRate_);
    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT,
                          &channelCount_);
    AMediaFormat_delete(format);
    return true; // Retry processing
  } else if (bufIdx == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
    return false;
  }

  return false;
}

void Mp3Decoder::seekTo(int64_t positionMs) {
  if (!extractor_ || !codec_)
    return;

  AMediaExtractor_seekTo(extractor_, positionMs * 1000,
                         AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC);
  AMediaCodec_flush(codec_);

  decodeBuffer_.clear();
  decodeBufferReadPtr_ = 0;
  sawInputEOS_ = false;
  sawOutputEOS_ = false;
  isEOS_ = false;
  currentPositionMs_ = positionMs;
}
