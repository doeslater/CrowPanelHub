package com.example.crowpanelhub

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.hilt.lifecycle.viewmodel.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.example.crowpanelhub.ui.theme.CrowPanelHubTheme
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

@Composable
fun DiagnosticsRoot(modifier: Modifier = Modifier, viewModel: DiagnosticsViewModel = hiltViewModel()) {
    val state by viewModel.state.collectAsStateWithLifecycle()
    DiagnosticsScreen(state = state, modifier = modifier)
}

@Composable
fun DiagnosticsScreen(state: DiagnosticsState, modifier: Modifier = Modifier) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text("Diagnostics", style = MaterialTheme.typography.headlineSmall)
        Text("Status: ${state.statusText}", style = MaterialTheme.typography.bodyLarge)
        if (state.lastErrorText != null) {
            Text("Last error: ${state.lastErrorText}", style = MaterialTheme.typography.bodyLarge)
        }

        HorizontalDivider()
        Text("Event log (newest first)", style = MaterialTheme.typography.titleMedium)

        val timeFormat = remember { SimpleDateFormat("HH:mm:ss", Locale.getDefault()) }
        LazyColumn(modifier = Modifier.fillMaxWidth()) {
            items(state.logEntries.asReversed()) { entry ->
                Text(
                    text = "${timeFormat.format(Date(entry.timestamp))}  ${entry.message}",
                    style = MaterialTheme.typography.bodyMedium,
                )
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
private fun DiagnosticsScreenPreview() {
    CrowPanelHubTheme {
        DiagnosticsScreen(
            state = DiagnosticsState(
                statusText = "Connected",
                logEntries = listOf(
                    LogEntry(0L, "Not connected"),
                    LogEntry(1000L, "Connecting..."),
                    LogEntry(2000L, "Connected"),
                    LogEntry(3000L, "Sending image (15010 bytes)..."),
                    LogEntry(4000L, "Image sent successfully"),
                ),
            ),
        )
    }
}
