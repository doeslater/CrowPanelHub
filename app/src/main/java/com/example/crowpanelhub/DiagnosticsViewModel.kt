package com.example.crowpanelhub

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import javax.inject.Inject
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.onEach
import kotlinx.coroutines.flow.update

data class DiagnosticsState(
    val statusText: String = "Not connected",
    val lastErrorText: String? = null,
    val logEntries: List<LogEntry> = emptyList(),
)

/**
 * Read-only -- no Action/Event type, since this screen has nothing to trigger
 * and no one-time side effect to emit, just live connection state and the
 * event log to display.
 */
@HiltViewModel
class DiagnosticsViewModel @Inject constructor(
    usbSerialTransport: UsbSerialTransport,
) : ViewModel() {
    private val _state = MutableStateFlow(DiagnosticsState())
    val state: StateFlow<DiagnosticsState> = _state.asStateFlow()

    init {
        usbSerialTransport.connectionState
            .onEach { connectionState ->
                val lastErrorText = when (connectionState) {
                    is UsbConnectionState.ConnectFailed -> connectionState.reason
                    UsbConnectionState.PermissionDenied -> "Permission denied"
                    else -> null
                }
                _state.update { it.copy(statusText = connectionState.toStatusText(), lastErrorText = lastErrorText) }
            }
            .launchIn(viewModelScope)

        usbSerialTransport.logEntries
            .onEach { entries -> _state.update { it.copy(logEntries = entries) } }
            .launchIn(viewModelScope)
    }
}
