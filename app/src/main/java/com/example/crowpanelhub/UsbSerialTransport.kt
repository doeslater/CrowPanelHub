package com.example.crowpanelhub

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import androidx.core.content.ContextCompat
import com.hoho.android.usbserial.driver.UsbSerialPort
import com.hoho.android.usbserial.driver.UsbSerialProber
import dagger.hilt.android.qualifiers.ApplicationContext
import javax.inject.Inject
import javax.inject.Singleton
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withContext

private const val BAUD_RATE = 115200
private const val WRITE_TIMEOUT_MS = 5_000
private const val MAX_LOG_ENTRIES = 100

data class LogEntry(val timestamp: Long, val message: String)

sealed interface UsbConnectionState {
    data object Disconnected : UsbConnectionState
    data object RequestingPermission : UsbConnectionState
    data object PermissionDenied : UsbConnectionState
    data object Connecting : UsbConnectionState
    data object Connected : UsbConnectionState
    data class ConnectFailed(val reason: String) : UsbConnectionState
}

/** Shared by MainViewModel and DiagnosticsViewModel -- both just display this state as text. */
fun UsbConnectionState.toStatusText(): String = when (this) {
    UsbConnectionState.Disconnected -> "Not connected"
    UsbConnectionState.RequestingPermission -> "Requesting permission..."
    UsbConnectionState.PermissionDenied -> "Permission denied"
    UsbConnectionState.Connecting -> "Connecting..."
    UsbConnectionState.Connected -> "Connected"
    is UsbConnectionState.ConnectFailed -> "Connect failed: $reason"
}

/**
 * Talks to the CrowPanel board over USB serial via usb-serial-for-android. No
 * disconnect()/reconnect handling -- there's no Disconnect button in this app,
 * so the port just stays open once connected; if the cable is pulled, the next
 * sendImage() call simply fails.
 */
@Singleton
class UsbSerialTransport @Inject constructor(
    @ApplicationContext private val context: Context,
) {
    private val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    private var port: UsbSerialPort? = null

    private val _connectionState = MutableStateFlow<UsbConnectionState>(UsbConnectionState.Disconnected)
    val connectionState: StateFlow<UsbConnectionState> = _connectionState.asStateFlow()

    private val _logEntries = MutableStateFlow<List<LogEntry>>(emptyList())
    val logEntries: StateFlow<List<LogEntry>> = _logEntries.asStateFlow()

    suspend fun connect() = withContext(Dispatchers.IO) {
        val driver = UsbSerialProber.getDefaultProber().findAllDrivers(usbManager).firstOrNull()
        if (driver == null) {
            updateState(UsbConnectionState.ConnectFailed("No USB serial device found"))
            return@withContext
        }

        val device = driver.device
        if (!usbManager.hasPermission(device)) {
            updateState(UsbConnectionState.RequestingPermission)
            if (!requestPermission(device)) {
                updateState(UsbConnectionState.PermissionDenied)
                return@withContext
            }
        }

        updateState(UsbConnectionState.Connecting)
        try {
            val connection = usbManager.openDevice(device)
            if (connection == null) {
                updateState(UsbConnectionState.ConnectFailed("Could not open USB device"))
                return@withContext
            }
            val openedPort = driver.ports.first()
            openedPort.open(connection)
            openedPort.setParameters(BAUD_RATE, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)
            port = openedPort
            updateState(UsbConnectionState.Connected)
        } catch (e: Exception) {
            updateState(UsbConnectionState.ConnectFailed(e.message ?: "Unknown error"))
        }
    }

    suspend fun sendImage(payload: ByteArray): Result<Unit> = withContext(Dispatchers.IO) {
        val activePort = port
        if (activePort == null) {
            log("Send failed: not connected")
            return@withContext Result.failure(IllegalStateException("Not connected"))
        }
        log("Sending image (${payload.size} bytes)...")
        runCatching {
            activePort.write(WireFrame.build(payload), WRITE_TIMEOUT_MS)
        }.onSuccess {
            log("Image sent successfully")
        }.onFailure { e ->
            log("Send failed: ${e.message}")
        }
    }

    private fun updateState(state: UsbConnectionState) {
        _connectionState.value = state
        log(state.toStatusText())
    }

    private fun log(message: String) {
        _logEntries.update { (it + LogEntry(System.currentTimeMillis(), message)).takeLast(MAX_LOG_ENTRIES) }
    }

    /**
     * Bridges Android's async USB-permission dialog into a suspend call. Two
     * details matter here: FLAG_MUTABLE is required (not the usual
     * FLAG_IMMUTABLE) since the system needs to write EXTRA_PERMISSION_GRANTED
     * into the intent before rebroadcasting it -- an immutable intent silently
     * comes back with no extras, reading as "denied" even after tapping Allow.
     * RECEIVER_NOT_EXPORTED satisfies Android 14's mandatory exported flag
     * uniformly back to API 24, and is correct here since this is a private,
     * package-scoped action only our own PendingIntent ever fires.
     */
    private suspend fun requestPermission(device: UsbDevice): Boolean =
        suspendCancellableCoroutine { cont ->
            val action = "${context.packageName}.USB_PERMISSION"
            val permissionIntent = PendingIntent.getBroadcast(
                context,
                0,
                Intent(action).setPackage(context.packageName),
                PendingIntent.FLAG_MUTABLE,
            )
            val receiver = object : BroadcastReceiver() {
                override fun onReceive(receiverContext: Context, intent: Intent) {
                    if (intent.action != action) return
                    context.unregisterReceiver(this)
                    val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                    if (cont.isActive) cont.resumeWith(Result.success(granted))
                }
            }
            ContextCompat.registerReceiver(
                context,
                receiver,
                IntentFilter(action),
                ContextCompat.RECEIVER_NOT_EXPORTED,
            )
            cont.invokeOnCancellation { context.unregisterReceiver(receiver) }
            usbManager.requestPermission(device, permissionIntent)
        }
}
