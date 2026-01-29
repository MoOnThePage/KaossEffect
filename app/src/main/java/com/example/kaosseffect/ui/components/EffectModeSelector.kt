package com.example.kaosseffect.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

import com.example.kaosseffect.ui.theme.BitcrushLime
import com.example.kaosseffect.ui.theme.DelayCyan
import com.example.kaosseffect.ui.theme.FilterOrange
import com.example.kaosseffect.ui.theme.FlangerPurple
import com.example.kaosseffect.ui.theme.PhaserYellow
import com.example.kaosseffect.ui.theme.ReverbPurple
import com.example.kaosseffect.ui.theme.RingModBlueGrey

@Composable
fun EffectModeSelector(
    modifier: Modifier = Modifier,
    currentMode: Int,
    onModeSelected: (Int) -> Unit
) {
    val modes = listOf("FILTER", "CHORUS", "REVERB", "PHASER", "CRUSH", "RINGMOD")
    
    // Theme colors matching XYPad
    val modeColors =
        listOf(
            FilterOrange,
            DelayCyan,
            ReverbPurple,
            PhaserYellow,
            BitcrushLime,
            RingModBlueGrey
        )

    Column(
        modifier = modifier
            .fillMaxWidth()
            .padding(horizontal = 16.dp, vertical = 2.dp),
        verticalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        val rows = modes.chunked(3)
        rows.forEachIndexed { rowIndex, rowModes ->
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                rowModes.forEachIndexed { colIndex, title ->
                    val index = rowIndex * 3 + colIndex
                    val isSelected = currentMode == index
                    val color = modeColors[index]

                    OutlinedButton(
                        onClick = { onModeSelected(index) },
                        modifier = Modifier
                            .weight(1f)
                            .height(32.dp),
                        shape = RoundedCornerShape(8.dp),
                        colors = ButtonDefaults.outlinedButtonColors(
                            containerColor = if (isSelected) color.copy(alpha = 0.2f) else Color.Transparent,
                            contentColor = if (isSelected) color else MaterialTheme.colorScheme.onSurfaceVariant
                        ),
                        border = ButtonDefaults.outlinedButtonBorder.copy(
                            brush = androidx.compose.ui.graphics.SolidColor(
                                if (isSelected) color else MaterialTheme.colorScheme.outline
                            )
                        ),
                        contentPadding = PaddingValues(vertical = 2.dp)
                    ) {
                        Text(
                            text = title,
                            fontSize = 10.sp,
                            fontWeight = if (isSelected) FontWeight.Bold else FontWeight.Normal,
                            maxLines = 1
                        )
                    }
                }
            }
        }
    }
}
