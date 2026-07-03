package com.example.crowpanelhub

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint

/**
 * Renders demo text into a packed 1-bit bitmap -- the phone-side equivalent of
 * display_text/display_text.ino's hardcoded rows, but drawn on the phone with
 * Android's own Canvas/Paint and sent as a plain bitmap over the wire protocol,
 * same as any other image. No firmware change needed: receive_image.ino just
 * draws whatever bitmap it's given.
 */
object TextBitmap {
    private val ROWS = listOf(
        "CrowPanel Hub" to 48f,
        "ESP32-S3 E-Ink" to 36f,
        "400x300 1-bit BW" to 28f,
        "USB-Serial Link" to 32f,
        "Low Power Display" to 28f,
    )

    fun generate(): ByteArray {
        val bitmap = Bitmap.createBitmap(WireFrame.DISPLAY_WIDTH, WireFrame.DISPLAY_HEIGHT, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        canvas.drawColor(Color.WHITE)

        val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.BLACK }
        var y = 10f
        for ((text, textSize) in ROWS) {
            paint.textSize = textSize
            y += textSize
            canvas.drawText(text, 10f, y, paint)
            y += 8f
        }

        return bitmap.toPackedBits()
    }

    private fun Bitmap.toPackedBits(): ByteArray {
        val rowBytes = WireFrame.DISPLAY_WIDTH / 8
        val payload = ByteArray(WireFrame.PAYLOAD_SIZE)
        for (y in 0 until WireFrame.DISPLAY_HEIGHT) {
            for (byteIndex in 0 until rowBytes) {
                var byteValue = 0
                for (bit in 0 until 8) {
                    val x = byteIndex * 8 + bit
                    val isBlack = Color.red(getPixel(x, y)) < 128
                    if (isBlack) byteValue = byteValue or (1 shl (7 - bit))
                }
                payload[y * rowBytes + byteIndex] = byteValue.toByte()
            }
        }
        return payload
    }
}
