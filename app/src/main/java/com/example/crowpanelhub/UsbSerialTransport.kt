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
import java.util.concurrent.atomic.AtomicBoolean
import javax.inject.Inject
import javax.inject.Singleton
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withContext

private const val BAUD_RATE = 115200
private const val WRITE_TIMEOUT_MS = 5_000
private const val READ_ACK_TIMEOUT_MS = 3_000
private const val MAX_SEND_ATTEMPTS = 3
private const val RECONNECT_DELAY_MS = 2_000L
private const val RESET_PULSE_MS = 100L
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
 * Talks to the CrowPanel board over USB serial via usb-serial-for-android.
 * There's still no Disconnect button in this app -- the port just stays open
 * once connected -- but sendImage() reconnects on its own if a write fails,
 * since that's the signature of the flaky-cable link dropping mid-session
 * (see docs/usb-reliability-fix-plan.md).
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

    // Guards against two overlapping connect() calls (e.g. a double-tap on
    // Connect before connectionState first flips away from Disconnected) --
    // without this, both calls could race through the permission-request
    // BroadcastReceiver registration or both try to open the port.
    private val isConnecting = AtomicBoolean(false)

    suspend fun connect() = withContext(Dispatchers.IO) {
        if (!isConnecting.compareAndSet(false, true)) return@withContext
        try {
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
        } finally {
            isConnecting.set(false)
        }
    }

    /**
     * Writes one wire-protocol frame and waits for test_card.ino's ack line.
     * A write failure (the flaky-cable case) triggers a full reconnect and
     * retry, up to MAX_SEND_ATTEMPTS total. A write that succeeds but gets no
     * ack, or an explicit "frame dropped: ..." rejection, is NOT retried --
     * that's not a link problem, so it's surfaced as-is (see
     * docs/usb-reliability-fix-plan.md for why these are treated differently).
     * onStatus reports human-readable phase text as it happens, since the
     * caller has no other way to show real progress for a multi-attempt send.
     */
    suspend fun sendImage(payload: ByteArray, onStatus: (String) -> Unit = {}): Result<Unit> =
        withContext(Dispatchers.IO) {
            val frame = WireFrame.build(payload)
            for (attempt in 1..MAX_SEND_ATTEMPTS) {
                val activePort = port
                if (activePort == null) {
                    log("Send failed: not connected")
                    return@withContext Result.failure(IllegalStateException("Not connected"))
                }

                val attemptMessage = "Sending frame (attempt $attempt/$MAX_SEND_ATTEMPTS)..."
                onStatus(attemptMessage)
                log(attemptMessage)

                val writeFailure = runCatching { activePort.write(frame, WRITE_TIMEOUT_MS) }.exceptionOrNull()
                if (writeFailure != null) {
                    log("Write failed: ${writeFailure.message}")
                    if (attempt == MAX_SEND_ATTEMPTS) {
                        val message = "Failed after $MAX_SEND_ATTEMPTS attempts -- check the USB connection"
                        onStatus(message)
                        return@withContext Result.failure(IllegalStateException(message))
                    }
                    onStatus("USB error, reconnecting... (attempt ${attempt + 1}/$MAX_SEND_ATTEMPTS)")
                    delay(RECONNECT_DELAY_MS)
                    reconnect()
                    continue
                }

                onStatus("Sent -- waiting for board confirmation...")
                log("Frame written, awaiting ack...")
                val response = readAckLine(activePort)
                return@withContext when {
                    response == null -> {
                        log("No ack received within ${READ_ACK_TIMEOUT_MS}ms")
                        val message = "Board didn't confirm -- it may be running different firmware"
                        onStatus(message)
                        Result.failure(IllegalStateException(message))
                    }
                    response.startsWith("frame ok") -> {
                        log(response)
                        onStatus("Confirmed: frame ok")
                        Result.success(Unit)
                    }
                    response.startsWith("frame dropped") -> {
                        log(response)
                        val message = "Board rejected frame: ${response.removePrefix("frame dropped: ")}"
                        onStatus(message)
                        Result.failure(IllegalStateException(message))
                    }
                    else -> {
                        log("Unexpected response: $response")
                        val message = "Board didn't confirm -- it may be running different firmware"
                        onStatus(message)
                        Result.failure(IllegalStateException(message))
                    }
                }
            }
            // Unreachable in practice -- every branch above returns -- but the
            // compiler can't see that a bounded for-loop always exits early here.
            Result.failure(IllegalStateException("Failed after $MAX_SEND_ATTEMPTS attempts -- check the USB connection"))
        }

    /**
     * Reads lines until one is test_card.ino's actual ack/rejection
     * ("frame ok"/"frame dropped"), waiting up to READ_ACK_TIMEOUT_MS total.
     * Any other line is discarded, not returned -- a board that just reset
     * (e.g. from resetBoard(), or a prior send) still has its ROM/app boot
     * banner and the boot self-test's own debug prints in the pipe, and none
     * of that is the response to *this* frame.
     * usb-serial-for-android's read() returns whatever bytes are available
     * within its own timeout, which may be less than a full line, so this
     * accumulates across calls until a newline shows up.
     */
    private fun readAckLine(activePort: UsbSerialPort): String? {
        val deadline = System.currentTimeMillis() + READ_ACK_TIMEOUT_MS
        val buffer = ByteArray(256)
        val builder = StringBuilder()
        while (System.currentTimeMillis() < deadline) {
            val remaining = (deadline - System.currentTimeMillis()).toInt().coerceAtLeast(1)
            val bytesRead = runCatching { activePort.read(buffer, remaining) }.getOrDefault(0)
            if (bytesRead > 0) {
                builder.append(String(buffer, 0, bytesRead, Charsets.US_ASCII))
                while (true) {
                    val newlineIndex = builder.indexOf("\n")
                    if (newlineIndex < 0) break
                    val line = builder.substring(0, newlineIndex).trim()
                    builder.delete(0, newlineIndex + 1)
                    if (line.startsWith("frame ok") || line.startsWith("frame dropped")) return line
                }
            }
        }
        return null
    }

    /** Tears down the stale port and re-runs discovery + open, same as connect(). */
    private suspend fun reconnect() {
        log("Reconnecting...")
        runCatching { port?.close() }
        port = null
        connect()
    }

    /** Soft-reboots the board via a DTR/RTS pulse -- same trick serial_sender.py uses. No reflash. */
    suspend fun resetBoard(): Result<Unit> = withContext(Dispatchers.IO) {
        val activePort = port
        if (activePort == null) {
            log("Reset failed: not connected")
            return@withContext Result.failure(IllegalStateException("Not connected"))
        }
        log("Resetting board...")
        runCatching {
            activePort.setRTS(true)
            Thread.sleep(RESET_PULSE_MS)
            activePort.setRTS(false)
        }.onSuccess {
            log("Board reset")
        }.onFailure { e ->
            log("Reset failed: ${e.message}")
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
