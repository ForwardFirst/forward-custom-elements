package com.forwardfirst.mmorpg

import android.content.Context
import android.opengl.GLSurfaceView
import android.view.MotionEvent
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class GameSurfaceView(context: Context) : GLSurfaceView(context) {

    private val renderer = GameRenderer(context)

    // Touch tracking
    private var lastTouchX = 0f
    private var lastTouchY = 0f
    private var cameraPointer = -1
    private var prevPinchDist = -1f

    init {
        setEGLContextClientVersion(3)
        setEGLConfigChooser(8, 8, 8, 8, 24, 0)
        setRenderer(renderer)
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        // Camera drag on the right half of the screen (pointer not captured by HUD)
        when(event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                val idx = event.actionIndex
                val x = event.getX(idx); val y = event.getY(idx)
                if(event.pointerCount == 1) {
                    cameraPointer = event.getPointerId(idx)
                    lastTouchX = x; lastTouchY = y
                }
            }
            MotionEvent.ACTION_MOVE -> {
                if(event.pointerCount == 2) {
                    // Pinch-to-zoom
                    val dx = event.getX(0)-event.getX(1)
                    val dy = event.getY(0)-event.getY(1)
                    val dist = Math.sqrt((dx*dx+dy*dy).toDouble()).toFloat()
                    if(prevPinchDist > 0) {
                        queueEvent { GameBridge.nativeCameraZoom((prevPinchDist-dist)*0.02f) }
                    }
                    prevPinchDist = dist
                    cameraPointer = -1
                } else {
                    prevPinchDist = -1f
                    val idx = (0 until event.pointerCount)
                        .firstOrNull { event.getPointerId(it)==cameraPointer } ?: return true
                    val x = event.getX(idx); val y = event.getY(idx)
                    val dx = (x - lastTouchX) * 0.004f
                    val dy = (y - lastTouchY) * 0.003f
                    lastTouchX = x; lastTouchY = y
                    queueEvent { GameBridge.nativeCameraOrbit(dx, -dy) }
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP,
            MotionEvent.ACTION_CANCEL -> {
                cameraPointer = -1; prevPinchDist = -1f
            }
        }
        return true
    }

    inner class GameRenderer(private val ctx: Context) : Renderer {
        private var lastTime = System.nanoTime()

        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            GameBridge.nativeInit(ctx.assets, 0, 0)
            lastTime = System.nanoTime()
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
            GameBridge.nativeResize(width, height)
        }

        override fun onDrawFrame(gl: GL10?) {
            val now = System.nanoTime()
            val dt  = ((now - lastTime) / 1_000_000_000.0).toFloat().coerceIn(0f, 0.1f)
            lastTime = now
            GameBridge.nativeFrame(dt)
        }
    }
}
