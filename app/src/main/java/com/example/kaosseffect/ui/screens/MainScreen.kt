package com.example.kaosseffect.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.foundation.layout.Row
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.kaosseffect.ui.components.EffectModeSelector
import com.example.kaosseffect.ui.components.TransportControls
import com.example.kaosseffect.ui.components.XYPad
import com.example.kaosseffect.viewmodel.AudioViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    viewModel: AudioViewModel = androidx.lifecycle.viewmodel.compose.viewModel(),
    onPickFile: () -> Unit = {}
) {
    val uiState by viewModel.uiState.collectAsState()

    // Format time helper
    fun formatTime(ms: Long): String {
        val totalSeconds = ms / 1000
        val minutes = totalSeconds / 60
        val seconds = totalSeconds % 60
        return "${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}"
    }

    val snackbarHostState = androidx.compose.runtime.remember { androidx.compose.material3.SnackbarHostState() }

    // Show error message if preset
    LaunchedEffect(uiState.errorMessage) {
        uiState.errorMessage?.let { message ->
            snackbarHostState.showSnackbar(
                message = message,
                duration = androidx.compose.material3.SnackbarDuration.Short
            )
            viewModel.clearError()
        }
    }

    Scaffold(
        modifier = Modifier.fillMaxSize(),
        snackbarHost = { androidx.compose.material3.SnackbarHost(hostState = snackbarHostState) },
        topBar = {
            TopAppBar(
                title = { 
                    Column {
                        Text(
                            text = uiState.fileName ?: "No file loaded",
                            style = MaterialTheme.typography.titleMedium,
                            maxLines = 1,
                            overflow = androidx.compose.ui.text.style.TextOverflow.Ellipsis
                        )
                        Text(
                            text = "${formatTime(uiState.currentPositionMs)} / ${formatTime(uiState.durationMs)}",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                },
                actions = {
                    IconButton(onClick = onPickFile) {
                        Icon(Icons.Default.Folder, contentDescription = "Open File")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.surfaceVariant
                )
            )
        }
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
                .background(MaterialTheme.colorScheme.background),
            verticalArrangement = Arrangement.SpaceBetween,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            // XY Pad Section
            XYPad(
                modifier = Modifier
                    .weight(1f)
                    .padding(16.dp),
                onPositionChanged = { x, y ->
                    viewModel.setXY(x, y)
                },
                effectMode = uiState.effectMode,
                visualizerData = uiState.visualizerData
            )

            // Controls Section
            Column(
                modifier = Modifier.fillMaxWidth()
            ) {
                if (uiState.isLoading) {
                    Box(modifier = Modifier.fillMaxWidth().padding(16.dp), contentAlignment = Alignment.Center) {
                         CircularProgressIndicator()
                    }
                }
                
                // Seekbar (Only if file loaded)
                if (uiState.durationMs > 0) {
                     var sliderPosition by remember(uiState.currentPositionMs) { 
                         mutableFloatStateOf(uiState.currentPositionMs.toFloat()) 
                     }
                     var isDragging by remember { mutableStateOf(false) }
                     var wasPlayingBeforeDrag by remember { mutableStateOf(false) }

                     // Sync slider with playback only if not dragging
                     LaunchedEffect(uiState.currentPositionMs) {
                         if (!isDragging) {
                             sliderPosition = uiState.currentPositionMs.toFloat()
                         }
                     }

                    Text(
                        text = "Seek Position",
                        style = MaterialTheme.typography.labelMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.padding(start = 24.dp, top = 8.dp)
                    )

                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 16.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            text = formatTime(sliderPosition.toLong()),
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurface,
                            modifier = Modifier.width(40.dp)
                        )

                        Slider(
                            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                            value = sliderPosition,
                            onValueChange = { 
                                if (!isDragging) {
                                    isDragging = true
                                    wasPlayingBeforeDrag = uiState.isPlaying
                                    if (wasPlayingBeforeDrag) {
                                        viewModel.pause()
                                    }
                                }
                                sliderPosition = it
                            },
                            onValueChangeFinished = {
                                viewModel.seekTo(sliderPosition.toLong())
                                if (wasPlayingBeforeDrag) {
                                    viewModel.play()
                                }
                                isDragging = false
                            },
                            valueRange = 0f..uiState.durationMs.toFloat().coerceAtLeast(1f),
                            colors = SliderDefaults.colors(
                                thumbColor = MaterialTheme.colorScheme.primary,
                                activeTrackColor = MaterialTheme.colorScheme.primary
                            )
                        )

                        Text(
                            text = formatTime(uiState.durationMs),
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurface,
                            modifier = Modifier.width(40.dp)
                        )
                    }
                }

                EffectModeSelector(
                    currentMode = uiState.effectMode,
                    onModeSelected = { mode ->
                        if (uiState.effectMode == mode) {
                           viewModel.setEffectMode(-1)
                        } else {
                           viewModel.setEffectMode(mode)
                        }
                    }
                )

                TransportControls(
                    isPlaying = uiState.isPlaying,
                    onPlayPause = {
                        if (uiState.isPlaying) {
                            viewModel.pause()
                        } else {
                            viewModel.play()
                        }
                    },
                    onStop = {
                        viewModel.stop()
                    }
                )
            }
        }
    }
}
