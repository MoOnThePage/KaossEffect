#include "AudioEngine.h"
#include <jni.h>
#include <string>

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeStart(JNIEnv *env,
                                                     jobject /* this */) {
  try {
    AudioEngine::getInstance()->startStream();
  } catch (...) {
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeStop(JNIEnv *env,
                                                    jobject /* this */) {
  try {
    AudioEngine::getInstance()->stopStream();
  } catch (...) {
  }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeLoadFile(JNIEnv *env,
                                                        jobject /* this */,
                                                        jint fd, jlong offset,
                                                        jlong size) {
  try {
    return AudioEngine::getInstance()->loadFile(fd, offset, size);
  } catch (...) {
    return false;
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativePlay(JNIEnv *env,
                                                    jobject /* this */) {
  try {
    AudioEngine::getInstance()->play();
  } catch (...) {
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativePause(JNIEnv *env,
                                                     jobject /* this */) {
  try {
    AudioEngine::getInstance()->pause();
  } catch (...) {
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeRewindStop(JNIEnv *env,
                                                          jobject /* this */) {
  try {
    AudioEngine::getInstance()->stop();
  } catch (...) {
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeSeekTo(JNIEnv *env,
                                                      jobject /* this */,
                                                      jlong positionMs) {
  try {
    AudioEngine::getInstance()->seekTo(positionMs);
  } catch (...) {
  }
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeGetDurationMs(
    JNIEnv *env, jobject /* this */) {
  try {
    return AudioEngine::getInstance()->getDurationMs();
  } catch (...) {
    return 0;
  }
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeGetPositionMs(
    JNIEnv *env, jobject /* this */) {
  try {
    return AudioEngine::getInstance()->getPositionMs();
  } catch (...) {
    return 0;
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeSetXY(JNIEnv *env,
                                                     jobject /* this */,
                                                     jint slot, jfloat x,
                                                     jfloat y) {
  try {
    AudioEngine::getInstance()->setXY(slot, x, y);
  } catch (...) {
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeSetEffectMode(JNIEnv *env,
                                                             jobject /* this */,
                                                             jint slot,
                                                             jint mode) {
  try {
    AudioEngine::getInstance()->setEffectMode(slot, mode);
  } catch (...) {
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeSetWetMix(JNIEnv *env,
                                                         jobject /* this */,
                                                         jint slot,
                                                         jfloat mix) {
  try {
    AudioEngine::getInstance()->setWetMix(slot, mix);
  } catch (...) {
  }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeIsPlaying(JNIEnv *env,
                                                         jobject /* this */) {
  try {
    return AudioEngine::getInstance()->isPlaying();
  } catch (...) {
    return false;
  }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_kaosseffect_AudioBridge_nativeGetVisualizerData(
    JNIEnv *env, jobject /* this */, jfloatArray buffer) {
  try {
    jfloat *nativeBuffer = env->GetFloatArrayElements(buffer, nullptr);
    jint length = env->GetArrayLength(buffer);
    int copied =
        AudioEngine::getInstance()->getVisualizerData(nativeBuffer, length);
    env->ReleaseFloatArrayElements(buffer, nativeBuffer, 0);
    return copied;
  } catch (...) {
    return 0;
  }
}
