KaossEffect: Android Audio Effects Processor
Application Overview
KaossEffect is an Android application that functions as an interactive audio effects processor inspired by the Korg Kaoss Pad. Users load MP3 files from their device, play them back, and apply real-time audio effects by touching and dragging on an XY pad interface.
Core Concept
The app mimics the tactile, performative experience of hardware effect processors. The X-axis controls one effect parameter (e.g., filter cutoff), while the Y-axis controls another (e.g., resonance). Users can switch between different effect modes, each mapping the XY coordinates to different parameters.

Functional Requirements
Audio Playback

Load MP3 files from device storage using Android's document picker
Decode MP3 files using Android MediaCodec NDK API
Output audio with low latency using Google Oboe library
Transport controls: Play, Pause, Stop
Display current playback position and file name

XY Pad Effects Controller

Full-screen touch area that tracks finger position
Normalized coordinates (0.0 to 1.0 on both axes)
Visual feedback showing current touch position
Smooth parameter interpolation to avoid audio artifacts
Multi-touch: use first touch only, ignore additional fingers

Effect Modes

Filter — X: Cutoff frequency (50Hz–15kHz exponential), Y: Resonance (0–95%)
Delay — X: Delay time (10ms–500ms), Y: Feedback (0–90%)
Bitcrusher — X: Bit depth (2–16 bits), Y: Sample rate reduction (1x–16x)
Flanger — X: Rate (0.1Hz–10Hz), Y: Depth (0–100%)

User Interface

Modern Material 3 design using Jetpack Compose
Dark theme optimized for performance use
Bottom navigation or tabs for effect mode selection
Minimal chrome — XY pad should dominate the screen
File info and transport controls in a collapsible top bar


Technical Architecture
┌─────────────────────────────────────────────────────────────┐
│                     Kotlin/Compose UI Layer                  │
│  ┌─────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │ File Picker │  │    XY Pad       │  │ Transport/Mode  │  │
│  │  Activity   │  │   Composable    │  │   Controls      │  │
│  └─────────────┘  └─────────────────┘  └─────────────────┘  │
└───────────────────────────┬─────────────────────────────────┘
                            │ JNI Bridge
┌───────────────────────────┴─────────────────────────────────┐
│                     C++ Native Layer                         │
│  ┌─────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │ Mp3Decoder  │──│  AudioEngine    │──│  Oboe Stream    │  │
│  │(MediaCodec) │  │                 │  │    Output       │  │
│  └─────────────┘  └────────┬────────┘  └─────────────────┘  │
│                            │                                 │
│                   ┌────────┴────────┐                       │
│                   │  Effects Chain  │                       │
│                   │ Filter│Delay│...│                       │
│                   └─────────────────┘                       │
└─────────────────────────────────────────────────────────────┘
Technology Stack

Language: Kotlin (UI), C++17 (audio)
UI Framework: Jetpack Compose with Material 3
Audio I/O: Google Oboe 1.8+
Audio Decoding: Android NDK MediaCodec API
Build System: Gradle with CMake for native code
Minimum SDK: API 24 (Android 7.0)
Target SDK: API 34 (Android 14)


Project Structure
app/
├── src/main/
│   ├── java/com/example/kaosseffect/
│   │   ├── MainActivity.kt
│   │   ├── AudioBridge.kt              # JNI interface
│   │   ├── ui/
│   │   │   ├── theme/
│   │   │   │   └── Theme.kt
│   │   │   ├── components/
│   │   │   │   ├── XYPad.kt
│   │   │   │   ├── TransportControls.kt
│   │   │   │   └── EffectModeSelector.kt
│   │   │   └── screens/
│   │   │       └── MainScreen.kt
│   │   └── viewmodel/
│   │       └── AudioViewModel.kt
│   ├── cpp/
│   │   ├── CMakeLists.txt
│   │   ├── native-lib.cpp              # JNI entry points
│   │   ├── AudioEngine.h
│   │   ├── AudioEngine.cpp
│   │   ├── Mp3Decoder.h
│   │   ├── Mp3Decoder.cpp
│   │   └── effects/
│   │       ├── Effect.h                # Base class
│   │       ├── Filter.h / Filter.cpp
│   │       ├── Delay.h / Delay.cpp
│   │       ├── Bitcrusher.h / Bitcrusher.cpp
│   │       └── Flanger.h / Flanger.cpp
│   └── res/
│       └── ... (standard Android resources)
├── build.gradle.kts
└── proguard-rules.pro

Data Flow

User selects MP3 file via document picker
Kotlin layer obtains file descriptor and passes path to native layer via JNI
Mp3Decoder opens file with AMediaExtractor, creates AMediaCodec decoder
AudioEngine starts Oboe output stream with callback
On each audio callback:

Decoder fills buffer with PCM samples
Active effect processes samples in-place
Processed samples written to Oboe output


User touches XY pad → Compose tracks position → JNI call updates atomic parameters
Effect reads parameters on next audio callback (lock-free)


Key Implementation Details
Thread Safety

Audio callback runs on high-priority thread — no locks, no allocations
Effect parameters use std::atomic<float> for lock-free updates
Decode buffer uses ring buffer or double-buffering pattern

Latency Optimization

Oboe configured for PerformanceMode::LowLatency
SharingMode::Exclusive for direct hardware access
Small buffer sizes (aim for <20ms round-trip)

Parameter Smoothing

Raw XY values smoothed with one-pole filter to prevent zipper noise
Smoothing coefficient ~0.99 for 1ms time constant at 44.1kHz

