package com.example.crowpanelhub

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.TimeZone

/**
 * Builds the wire-protocol frame receive_image.ino expects (see CLAUDE.md "Wire protocol"):
 * [1-byte magic 0xA5][4-byte LE payload length][4-byte LE epoch seconds]
 * [payload][1-byte checksum = 8-bit sum mod 256 of the payload bytes only].
 *
 * The firmware has no RTC/NTP and just formats whatever epoch seconds it's
 * given, with no timezone math of its own -- so the epoch value sent here is
 * the phone's real UTC time with its local UTC offset already added in,
 * making the panel's "last updated" label read as local wall-clock time
 * without the firmware ever needing to know about timezones.
 */
object WireFrame {
    private const val MAGIC: Byte = 0xA5.toByte()

    const val DISPLAY_WIDTH = 400
    const val DISPLAY_HEIGHT = 300
    const val PAYLOAD_SIZE = (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8 // 15,000 bytes, packed 1bpp

    fun build(
        payload: ByteArray,
        epochSeconds: Long = localEpochSeconds(),
    ): ByteArray {
        require(payload.size == PAYLOAD_SIZE) {
            "Payload must be exactly $PAYLOAD_SIZE bytes, was ${payload.size}"
        }
        return ByteBuffer.allocate(1 + 4 + 4 + payload.size + 1)
            .order(ByteOrder.LITTLE_ENDIAN)
            .put(MAGIC)
            .putInt(payload.size)
            .putInt(epochSeconds.toInt())
            .put(payload)
            .put(checksumOf(payload))
            .array()
    }

    private fun checksumOf(payload: ByteArray): Byte {
        var sum = 0
        for (b in payload) sum += (b.toInt() and 0xFF)
        return (sum and 0xFF).toByte()
    }

    private fun localEpochSeconds(): Long {
        val nowMillis = System.currentTimeMillis()
        val offsetMillis = TimeZone.getDefault().getOffset(nowMillis)
        return (nowMillis + offsetMillis) / 1000
    }
}
