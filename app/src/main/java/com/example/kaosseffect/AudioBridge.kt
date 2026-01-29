package com.example.kaosseffect

object AudioBridge {
    init {
        System.loadLibrary("native-lib")
    }

    private external fun nativeStart()
    private external fun nativeStop()
    private external fun nativeSetXY(slot: Int, x: Float, y: Float)
    private external fun nativeSetEffectMode(slot: Int, mode: Int)
    private external fun nativeSetWetMix(slot: Int, mix: Float)
    private external fun nativeIsPlaying(): Boolean
    
    private external fun nativeLoadFile(fd: Int, offset: Long, size: Long): Boolean
    private external fun nativePlay()
    private external fun nativePause()
    private external fun nativeRewindStop()
    private external fun nativeSeekTo(positionMs: Long)
    private external fun nativeGetDurationMs(): Long

    private external fun nativeGetPositionMs(): Long
    private external fun nativeGetVisualizerData(buffer: FloatArray): Int

    fun start() {
        nativeStart()
    }

    // Lifecycle stop
    fun stop() {
        nativeStop()
    }
    
    // Transport controls
    fun loadFile(fd: Int, offset: Long, size: Long): Boolean {
        return nativeLoadFile(fd, offset, size)
    }
    
    fun play() {
        nativePlay()
    }
    
    fun pause() {
        nativePause()
    }
    
    fun stopPlayback() {
        nativeRewindStop()
    }
    
    fun seekTo(positionMs: Long) {
        nativeSeekTo(positionMs)
    }
    
    fun getDurationMs(): Long {
        return nativeGetDurationMs()
    }
    
    fun getPositionMs(): Long {
        return nativeGetPositionMs()
    }

    fun getVisualizerData(buffer: FloatArray): Int {
        return nativeGetVisualizerData(buffer)
    }

    fun setXY(slot: Int, x: Float, y: Float) {
        nativeSetXY(slot, x, y)
    }

    fun setEffectMode(slot: Int, mode: Int) {
        nativeSetEffectMode(slot, mode)
    }

    fun setWetMix(slot: Int, mix: Float) {
        nativeSetWetMix(slot, mix)
    }

    fun isPlaying(): Boolean {
        return nativeIsPlaying()
    }
}
