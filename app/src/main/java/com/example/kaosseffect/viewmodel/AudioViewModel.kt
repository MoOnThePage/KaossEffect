package com.example.kaosseffect.viewmodel

import android.os.ParcelFileDescriptor
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.kaosseffect.AudioBridge
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

data class AudioUiState(
    val fileName: String? = null,
    val isPlaying: Boolean = false,
    val isLoading: Boolean = false,
    val currentPositionMs: Long = 0,
    val durationMs: Long = 0,
    val effectMode: Int = -1,
    val xyPosition: Pair<Float, Float> = 0.5f to 0.5f,
    val visualizerData: FloatArray = FloatArray(0),
    val errorMessage: String? = null
)

class AudioViewModel : ViewModel() {
    private val _uiState = MutableStateFlow(AudioUiState())
    val uiState: StateFlow<AudioUiState> = _uiState.asStateFlow()

    private var positionPollingJob: Job? = null
    private var currentPfd: ParcelFileDescriptor? = null

    init {
        // Start audio engine
        AudioBridge.start()
        // Start polling loop
        startPositionPolling()
    }

    private fun startPositionPolling() {
        positionPollingJob?.cancel()
        positionPollingJob = viewModelScope.launch {
            while (true) {
                if (_uiState.value.isPlaying) {
                    val pos = AudioBridge.getPositionMs()
                    val enginePlaying = AudioBridge.isPlaying()
                    
                    val visBuffer = FloatArray(256)
                    val points = AudioBridge.getVisualizerData(visBuffer)
                    
                    _uiState.update { currentState ->
                        val validData = if (points > 0) visBuffer.copyOfRange(0, points) else currentState.visualizerData
                        currentState.copy(
                            currentPositionMs = pos,
                            isPlaying = enginePlaying,
                            visualizerData = validData
                        )
                    }

                } else {
                    if (_uiState.value.visualizerData.isNotEmpty()) {
                        _uiState.update { it.copy(visualizerData = FloatArray(0)) }
                    }
                }
                delay(30) // Faster polling for smooth animation (was 100)
            }
        }
    }

    fun loadFile(pfd: ParcelFileDescriptor, name: String) {
        // Close previous file descriptor if exists
        currentPfd?.close()
        currentPfd = pfd
        
        _uiState.update { it.copy(isLoading = true) }
        
        // Launch in background/IO dispatcher to avoid blocking UI
        // Since AudioBridge calls interact with native, best to offload if heavy
        // But for now keeping it simple or moving to IO scope
        viewModelScope.launch(kotlinx.coroutines.Dispatchers.IO) {
            val fd = pfd.fd
            val size = pfd.statSize
            
            val success = AudioBridge.loadFile(fd, 0, size)
            if (success) {
                val duration = AudioBridge.getDurationMs()
                _uiState.update { it.copy(
                    fileName = name,
                    durationMs = duration,
                    currentPositionMs = 0,
                    isPlaying = false,
                    isLoading = false,
                    errorMessage = null
                ) }
            } else {
                 _uiState.update { it.copy(
                    fileName = "Error loading file",
                    isLoading = false,
                    errorMessage = "Failed to load audio file. Please check if the file is valid and supported."
                 ) }
            }
        }
    }
    
    fun clearError() {
        _uiState.update { it.copy(errorMessage = null) }
    }

    override fun onCleared() {
        super.onCleared()
        currentPfd?.close()
        AudioBridge.stopPlayback()
        AudioBridge.stop()
    }

    fun play() {
        AudioBridge.play()
        _uiState.update { it.copy(isPlaying = true) }
    }

    fun pause() {
        AudioBridge.pause()
        _uiState.update { it.copy(isPlaying = false) }
    }

    fun stop() {
        AudioBridge.stopPlayback()
        _uiState.update { it.copy(isPlaying = false, currentPositionMs = 0) }
    }

    fun seekTo(positionMs: Long) {
        AudioBridge.seekTo(positionMs)
        _uiState.update { it.copy(currentPositionMs = positionMs) }
    }

    fun setXY(x: Float, y: Float) {
        AudioBridge.setXY(x, y)
        _uiState.update { it.copy(xyPosition = x to y) }
    }

    fun setEffectMode(mode: Int) {
        AudioBridge.setEffectMode(mode)
        _uiState.update { it.copy(effectMode = mode) }
    }
}
