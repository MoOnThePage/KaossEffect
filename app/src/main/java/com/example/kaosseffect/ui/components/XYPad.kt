package com.example.kaosseffect.ui.components

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.Box
import androidx.compose.ui.Alignment
import androidx.compose.material3.Text
import androidx.compose.material3.MaterialTheme

import com.example.kaosseffect.ui.theme.BitcrushLime
import com.example.kaosseffect.ui.theme.DelayCyan
import com.example.kaosseffect.ui.theme.FilterOrange
import com.example.kaosseffect.ui.theme.FlangerPurple

@Composable
fun XYPad(
    modifier: Modifier = Modifier,
    onPositionChanged: (x: Float, y: Float) -> Unit,
    effectMode: Int = 0
) {
    // State for normalized position (0.0 - 1.0)
    var currentX by remember { mutableFloatStateOf(0.5f) }
    var currentY by remember { mutableFloatStateOf(0.5f) }
    var isTouching by remember { mutableStateOf(false) }

    val view = androidx.compose.ui.platform.LocalView.current
    
    // Indicator opacity animation
    val indicatorAlpha by animateFloatAsState(
        targetValue = if (isTouching) 1f else 0.5f,
        animationSpec = tween(durationMillis = 300),
        label = "Indicator Alpha"
    )

    // Helper to update position ensuring bounds
    fun updatePosition(width: Float, height: Float, touchX: Float, touchY: Float) {
        val newX = (touchX / width).coerceIn(0f, 1f)
        val newY = (touchY / height).coerceIn(0f, 1f)
        
        currentX = newX
        currentY = newY
        
        onPositionChanged(currentX, currentY)
    }

    // Effect Metadata
    data class EffectInfo(val xName: String, val yName: String, val xValue: String, val yValue: String)
    val effectInfo = remember(effectMode, currentX, currentY) {
        when (effectMode) {
            0 -> { // Filter
                // 20Hz - 20kHz exp
                val freq = 20.0 * Math.pow(1000.0, currentX.toDouble())
                val res = currentY * 0.95
                EffectInfo("Cutoff", "Resonance", "%.0f Hz".format(freq), "%.2f".format(res))
            }
            1 -> { // Delay
                // 10ms - 500ms
                val time = 10.0 + (490.0 * currentX)
                val fb = currentY * 0.9
                EffectInfo("Time", "Feedback", "%.0f ms".format(time), "%.2f".format(fb))
            }
            2 -> { // Bitcrusher
                // 16 -> 2 bits
                val bits = 16.0 - (currentX * 14.0)
                // 1 -> 32 div
                val rate = 1.0 + (currentY * 31.0)
                EffectInfo("Bit Depth", "Rate Div", "%.1f bits".format(bits), "1/%.0f".format(rate))
            }
            3 -> { // Flanger
                // 0.05 - 5Hz
                val rate = 0.05 * Math.pow(100.0, currentX.toDouble())
                val depth = currentY
                EffectInfo("LFO Rate", "Depth", "%.2f Hz".format(rate), "%.2f".format(depth))
            }
            else -> EffectInfo("X", "Y", "%.2f".format(currentX), "%.2f".format(currentY))
        }
    }

    // Colors based on effect mode
    val themeColor = remember(effectMode) {
        when (effectMode) {
            0 -> FilterOrange
            1 -> DelayCyan
            2 -> BitcrushLime
            3 -> FlangerPurple
            else -> Color.White
        }
    }

    val gridColor = themeColor.copy(alpha = 0.3f)
    val crosshairColor = themeColor.copy(alpha = 0.6f)

    BoxWithConstraints(
        modifier = modifier
            .fillMaxWidth()
            .aspectRatio(1f) // Force square aspect ratio
            .pointerInput(Unit) {
                detectDragGestures(
                    onDragStart = { offset ->
                        isTouching = true
                        view.performHapticFeedback(android.view.HapticFeedbackConstants.KEYBOARD_TAP)
                        updatePosition(size.width.toFloat(), size.height.toFloat(), offset.x, offset.y)
                    },
                    onDragEnd = { isTouching = false },
                    onDragCancel = { isTouching = false },
                    onDrag = { change, _ ->
                        change.consume()
                        updatePosition(size.width.toFloat(), size.height.toFloat(), change.position.x, change.position.y)
                    }
                )
            }
            .pointerInput(Unit) {
                detectTapGestures(
                    onPress = { offset ->
                        isTouching = true
                        view.performHapticFeedback(android.view.HapticFeedbackConstants.KEYBOARD_TAP)
                        updatePosition(size.width.toFloat(), size.height.toFloat(), offset.x, offset.y)
                        tryAwaitRelease()
                        isTouching = false
                    }
                )
            }
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val width = size.width
            val height = size.height

            // 1. Draw Background Grid (8x8)
            val rows = 8
            val cols = 8
            
            // Draw Vertical lines
            for (i in 1 until cols) {
                val x = (width / cols) * i
                drawLine(
                    color = gridColor,
                    start = Offset(x, 0f),
                    end = Offset(x, height),
                    strokeWidth = 1.dp.toPx()
                )
            }

            // Draw Horizontal lines
            for (i in 1 until rows) {
                val y = (height / rows) * i
                drawLine(
                    color = gridColor,
                    start = Offset(0f, y),
                    end = Offset(width, y),
                    strokeWidth = 1.dp.toPx()
                )
            }

            // Draw Border
            drawRect(
                color = gridColor,
                style = Stroke(width = 2.dp.toPx())
            )

            // Calculate active position in pixels
            val activeX = currentX * width
            val activeY = currentY * height

            // 2. Draw Crosshairs
            drawLine(
                color = crosshairColor,
                start = Offset(activeX, 0f),
                end = Offset(activeX, height),
                strokeWidth = 2.dp.toPx()
            )
            drawLine(
                color = crosshairColor,
                start = Offset(0f, activeY),
                end = Offset(width, activeY),
                strokeWidth = 2.dp.toPx()
            )

            // 3. Draw Touch Indicator with Ripple
            drawCircle(
                color = themeColor.copy(alpha = indicatorAlpha * 0.3f),
                radius = 36.dp.toPx() * (0.8f + 0.2f * indicatorAlpha), // Breath
                center = Offset(activeX, activeY)
            )

            drawCircle(
                color = themeColor.copy(alpha = indicatorAlpha),
                radius = 24.dp.toPx(),
                center = Offset(activeX, activeY)
            )
            
            // Inner core of indicator
            drawCircle(
                color = Color.White.copy(alpha = 0.8f * indicatorAlpha),
                radius = 8.dp.toPx(),
                center = Offset(activeX, activeY)
            )
        }
        
        // 4. Parameter Labels
        // Bottom-Right: X Parameter
        // 4. Parameter Labels
        // Bottom-Right: X Parameter
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp),
            contentAlignment = Alignment.BottomEnd
        ) {
            Text(
                text = "${effectInfo.xName}: ${effectInfo.xValue}",
                style = MaterialTheme.typography.labelLarge,
                color = themeColor
            )
        }
        
        // Top-Left: Y Parameter
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp),
            contentAlignment = Alignment.TopStart
        ) {
            Text(
                text = "${effectInfo.yName}: ${effectInfo.yValue}",
                style = MaterialTheme.typography.labelLarge,
                color = themeColor
            )
        }
    }
}
