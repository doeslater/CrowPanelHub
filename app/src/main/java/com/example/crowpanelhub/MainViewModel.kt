package com.example.crowpanelhub

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import javax.inject.Inject
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.onEach
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

// The wire protocol is fire-and-forget (see CLAUDE.md) -- the ESP32 never
// confirms when it's actually done redrawing the panel, only that bytes
// arrived. A full e-paper refresh on this panel typically takes several
// seconds; this is a rough estimate to keep Send disabled long enough that a
// second tap doesn't land while the firmware is still blocked inside
// epd.display() and not reading Serial, which would corrupt/drop that frame.
private const val ESTIMATED_WRITE_TIME_MS = 1_300L // ~15KB over 115200 baud
private const val ESTIMATED_PANEL_REFRESH_COOLDOWN_MS = 4_000L
private const val ESTIMATED_TOTAL_SEND_TIME_MS = ESTIMATED_WRITE_TIME_MS + ESTIMATED_PANEL_REFRESH_COOLDOWN_MS
private const val PROGRESS_TICK_MS = 100L

enum class TransportOption(val label: String) { USB("USB") }

data class MainState(
    val transportOptions: List<TransportOption> = TransportOption.entries,
    val selectedTransport: TransportOption = TransportOption.USB,
    val statusText: String = "Not connected",
    val isConnected: Boolean = false,
    val isBusy: Boolean = false,
    val progressFraction: Float = 0f,
)

sealed interface MainAction {
    data class OnTransportSelected(val option: TransportOption) : MainAction
    data object OnConnectClick : MainAction
    data object OnSendTextClick : MainAction
    data object OnSendCheckerboardClick : MainAction
}

@HiltViewModel
class MainViewModel @Inject constructor(
    private val usbSerialTransport: UsbSerialTransport,
) : ViewModel() {
    private val _state = MutableStateFlow(MainState())
    val state: StateFlow<MainState> = _state.asStateFlow()

    init {
        usbSerialTransport.connectionState
            .onEach { connectionState ->
                _state.update {
                    it.copy(
                        statusText = connectionState.toStatusText(),
                        isConnected = connectionState is UsbConnectionState.Connected,
                        isBusy = connectionState is UsbConnectionState.Connecting ||
                            connectionState is UsbConnectionState.RequestingPermission,
                    )
                }
            }
            .launchIn(viewModelScope)
    }

    fun onAction(action: MainAction) {
        when (action) {
            is MainAction.OnTransportSelected -> _state.update { it.copy(selectedTransport = action.option) }
            MainAction.OnConnectClick -> viewModelScope.launch { usbSerialTransport.connect() }
            MainAction.OnSendTextClick -> send(TextBitmap.generate())
            MainAction.OnSendCheckerboardClick -> send(Checkerboard.generate())
        }
    }

    private fun send(payload: ByteArray) {
        viewModelScope.launch {
            _state.update { it.copy(statusText = "Sending...", isBusy = true, progressFraction = 0f) }

            // Ticks progress across the whole estimated write+cooldown window,
            // independent of exactly when the real operation below finishes --
            // it's an estimate either way, so this keeps the bar simple.
            val progressJob = launch {
                var elapsedMs = 0L
                while (elapsedMs < ESTIMATED_TOTAL_SEND_TIME_MS) {
                    delay(PROGRESS_TICK_MS)
                    elapsedMs += PROGRESS_TICK_MS
                    val fraction = (elapsedMs.toFloat() / ESTIMATED_TOTAL_SEND_TIME_MS).coerceAtMost(1f)
                    _state.update { it.copy(progressFraction = fraction) }
                }
            }

            usbSerialTransport.sendImage(payload)
                .onSuccess {
                    _state.update { it.copy(statusText = "Sent, panel refreshing...") }
                    delay(ESTIMATED_PANEL_REFRESH_COOLDOWN_MS)
                    progressJob.cancel()
                    _state.update { it.copy(statusText = "Sent successfully", isBusy = false, progressFraction = 0f) }
                }
                .onFailure { e ->
                    progressJob.cancel()
                    _state.update { it.copy(statusText = "Send failed: ${e.message}", isBusy = false, progressFraction = 0f) }
                }
        }
    }
}
