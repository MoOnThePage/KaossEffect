package com.example.kaosseffect

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import com.example.kaosseffect.ui.screens.MainScreen
import com.example.kaosseffect.ui.theme.KaossEffectTheme
import com.example.kaosseffect.viewmodel.AudioViewModel
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.lifecycle.Lifecycle
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {
    private val viewModel: AudioViewModel by viewModels()

    private val openDocumentLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            result.data?.data?.let { uri ->
                handleFileSelection(uri)
            }
        }
    }

    private lateinit var audioManager: android.media.AudioManager
    private val focusRequest = android.media.AudioAttributes.Builder()
        .setUsage(android.media.AudioAttributes.USAGE_MEDIA)
        .setContentType(android.media.AudioAttributes.CONTENT_TYPE_MUSIC)
        .build()
        .let { attributes ->
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
                android.media.AudioFocusRequest.Builder(android.media.AudioManager.AUDIOFOCUS_GAIN)
                    .setAudioAttributes(attributes)
                    .setAcceptsDelayedFocusGain(true)
                    .setOnAudioFocusChangeListener { focusChange ->
                        handleAudioFocusChange(focusChange)
                    }
                    .build()
            } else {
                null
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        audioManager = getSystemService(android.content.Context.AUDIO_SERVICE) as android.media.AudioManager

        // Observe playback state to manage focus
        // Observe playback state to manage focus
        // Observe playback state to manage focus
        lifecycleScope.launch {
             viewModel.uiState.collect { state ->
                if (state.isPlaying) {
                    requestAudioFocus()
                } else {
                    abandonAudioFocus()
                }
            }
        }

        setContent {
            KaossEffectTheme {
                MainScreen(
                    viewModel = viewModel,
                    onPickFile = { launchFilePicker() }
                )
            }
        }
    }

    private fun requestAudioFocus() {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            focusRequest?.let { audioManager.requestAudioFocus(it) }
        } else {
            @Suppress("DEPRECATION")
            audioManager.requestAudioFocus(
                { focusChange -> handleAudioFocusChange(focusChange) },
                android.media.AudioManager.STREAM_MUSIC,
                android.media.AudioManager.AUDIOFOCUS_GAIN
            )
        }
    }

    private fun abandonAudioFocus() {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            focusRequest?.let { audioManager.abandonAudioFocusRequest(it) }
        } else {
            @Suppress("DEPRECATION")
            audioManager.abandonAudioFocus { /* listener */ }
        }
    }

    private fun handleAudioFocusChange(focusChange: Int) {
        when (focusChange) {
            android.media.AudioManager.AUDIOFOCUS_LOSS -> {
                viewModel.pause()
            }
            android.media.AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
                viewModel.pause()
            }
            android.media.AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
                // Lower volume or pause, we'll pause for simplicity
                viewModel.pause()
            }
            android.media.AudioManager.AUDIOFOCUS_GAIN -> {
                // Optionally resume? Best not to auto-resume unexpectedly.
            }
        }
    }
    
    // ... (rest of file picker logic)
    private fun launchFilePicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "audio/*"
        }
        openDocumentLauncher.launch(intent)
    }

    private fun handleFileSelection(uri: Uri) {
        val contentResolver = applicationContext.contentResolver
        
        // Get file name
        var displayName = "Unknown"
        contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                if (nameIndex != -1) {
                    displayName = cursor.getString(nameIndex)
                }
            }
        }

        // Get File Descriptor
        try {
            contentResolver.openFileDescriptor(uri, "r")?.let { pfd ->
                viewModel.loadFile(pfd, displayName)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    override fun onResume() {
        super.onResume()
    }

    override fun onPause() {
        super.onPause()
        // Determine if we should pause on app background or keep playing?
        // "Background app -> audio continues" request implies we should NOT pause in onPause.
        // So I'll remove the automatic pause here.
        // But we must respect Audio Focus.
    }
    
    override fun onDestroy() {
        super.onDestroy()
        abandonAudioFocus()
    }
}
