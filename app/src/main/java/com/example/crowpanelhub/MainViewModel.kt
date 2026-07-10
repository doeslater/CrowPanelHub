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
import kotlinx.coroutines.launch

enum class TransportOption(val label: String) { USB("USB") }

data class MainState(
    val transportOptions: List<TransportOption> = TransportOption.entries,
    val selectedTransport: TransportOption = TransportOption.USB,
    val statusText: String = "Not connected",
    val isConnected: Boolean = false,
    val isBusy: Boolean = false,
)

sealed interface MainAction {
    data class OnTransportSelected(val option: TransportOption) : MainAction
    data object OnConnectClick : MainAction
    data object OnSendTextClick : MainAction
    data object OnSendCheckerboardClick : MainAction
    data object OnResetBoardClick : MainAction
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
            MainAction.OnResetBoardClick -> resetBoard()
        }
    }

    private fun send(payload: ByteArray) {
        viewModelScope.launch {
            _state.update { it.copy(isBusy = true) }
            usbSerialTransport.sendImage(payload) { status ->
                _state.update { it.copy(statusText = status) }
            }
            _state.update { it.copy(isBusy = false) }
        }
    }

    private fun resetBoard() {
        viewModelScope.launch {
            _state.update { it.copy(isBusy = true, statusText = "Resetting board...") }
            usbSerialTransport.resetBoard()
                .onSuccess { _state.update { it.copy(statusText = "Board reset") } }
                .onFailure { e -> _state.update { it.copy(statusText = "Reset failed: ${e.message}") } }
            _state.update { it.copy(isBusy = false) }
        }
    }
}
