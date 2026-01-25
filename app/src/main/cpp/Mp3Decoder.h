#ifndef KAOSSEFFECT_MP3DECODER_H
#define KAOSSEFFECT_MP3DECODER_H

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <string>
#include <vector>

class Mp3Decoder {
public:
  Mp3Decoder();
  ~Mp3Decoder();

  bool open(int fd, int64_t offset, int64_t size);
  int read(float *buffer, int samplesToRead);
  void seekTo(int64_t positionMs);
  void close();

  int64_t getDurationMs() const { return durationMs_; }
  int64_t getPositionMs() const { return currentPositionMs_; }
  bool isEndOfStream() const { return isEOS_; }
  int32_t getSampleRate() const { return sampleRate_; }
  int32_t getChannelCount() const { return channelCount_; }

private:
  AMediaExtractor *extractor_ = nullptr;
  AMediaCodec *codec_ = nullptr;

  std::vector<float> decodeBuffer_;
  size_t decodeBufferReadPtr_ = 0;

  int32_t sampleRate_ = 0;
  int32_t channelCount_ = 0;
  int64_t durationMs_ = 0;
  int64_t currentPositionMs_ = 0;

  bool isEOS_ = false;
  bool sawInputEOS_ = false;
  bool sawOutputEOS_ = false;

  // Helper to process output buffers
  bool processOutput();
};

#endif // KAOSSEFFECT_MP3DECODER_H
