package com.example.crowpanelhub

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.hilt.lifecycle.viewmodel.compose.hiltViewModel
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.example.crowpanelhub.ui.theme.CrowPanelHubTheme

@Composable
fun MainRoot(
    onNavigateToDiagnostics: () -> Unit,
    modifier: Modifier = Modifier,
    viewModel: MainViewModel = hiltViewModel(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()
    MainScreen(
        state = state,
        onAction = viewModel::onAction,
        onNavigateToDiagnostics = onNavigateToDiagnostics,
        modifier = modifier,
    )
}

@Composable
fun MainScreen(
    state: MainState,
    onAction: (MainAction) -> Unit,
    onNavigateToDiagnostics: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Button(onClick = onNavigateToDiagnostics) {
            Text("Diagnostics")
        }

        SingleChoiceSegmentedButtonRow {
            state.transportOptions.forEachIndexed { index, option ->
                SegmentedButton(
                    selected = state.selectedTransport == option,
                    onClick = { onAction(MainAction.OnTransportSelected(option)) },
                    shape = SegmentedButtonDefaults.itemShape(index, state.transportOptions.size),
                ) {
                    Text(option.label)
                }
            }
        }

        Button(
            onClick = { onAction(MainAction.OnConnectClick) },
            enabled = !state.isBusy,
        ) {
            Text("Connect")
        }

        Button(
            onClick = { onAction(MainAction.OnSendTextClick) },
            enabled = state.isConnected && !state.isBusy,
        ) {
            Text("Send Text")
        }

        Button(
            onClick = { onAction(MainAction.OnSendCheckerboardClick) },
            enabled = state.isConnected && !state.isBusy,
        ) {
            Text("Send Checkerboard")
        }

        Button(
            onClick = { onAction(MainAction.OnResetBoardClick) },
            enabled = state.isConnected && !state.isBusy,
        ) {
            Text("Reset Board")
        }

        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            if (state.isBusy) {
                CircularProgressIndicator()
            }

            Text(
                text = state.statusText,
                style = MaterialTheme.typography.titleLarge,
            )
        }
    }
}

@Preview(showBackground = true)
@Composable
private fun MainScreenPreview() {
    CrowPanelHubTheme {
        MainScreen(state = MainState(), onAction = {}, onNavigateToDiagnostics = {})
    }
}
