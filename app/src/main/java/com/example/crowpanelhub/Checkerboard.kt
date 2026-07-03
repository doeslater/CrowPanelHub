package com.example.crowpanelhub

/**
 * Generates the original milestone-1 test pattern: a packed 1-bit checkerboard,
 * matching receive_image/send_test_frame.py's checkerboard_payload() exactly.
 * Not currently wired into MainViewModel's Send action (that uses TextBitmap
 * instead) -- kept here as a second known-good pattern to swap in and play
 * with, e.g. via TextBitmap.generate() -> Checkerboard.generate() in
 * MainViewModel.onAction's OnSendClick branch.
 */
object Checkerboard {
    fun generate(): ByteArray {
        val rowBytes = WireFrame.DISPLAY_WIDTH / 8
        val payload = ByteArray(WireFrame.PAYLOAD_SIZE)
        for (y in 0 until WireFrame.DISPLAY_HEIGHT) {
            for (byteIndex in 0 until rowBytes) {
                var byteValue = 0
                for (bit in 0 until 8) {
                    val x = byteIndex * 8 + bit
                    val isOn = ((x / 8) + (y / 8)) % 2 == 0
                    if (isOn) byteValue = byteValue or (1 shl (7 - bit))
                }
                payload[y * rowBytes + byteIndex] = byteValue.toByte()
            }
        }
        return payload
    }
}
