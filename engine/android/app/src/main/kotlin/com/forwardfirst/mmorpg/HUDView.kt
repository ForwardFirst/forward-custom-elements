package com.forwardfirst.mmorpg

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View
import kotlin.math.hypot
import kotlin.math.min

@SuppressLint("ViewConstructor")
class HUDView(context: Context, private val gameView: GameSurfaceView) : View(context) {

    // ---- Joystick ----
    private val stickBaseRadius = 90f
    private val stickKnobRadius = 40f
    private var stickBaseX = 0f; private var stickBaseY = 0f
    private var stickKnobX = 0f; private var stickKnobY = 0f
    private var stickPointerId = -1
    private var stickActive = false

    // ---- Buttons ----
    private val btnSize = 80f
    private var attackBtnCx = 0f; private var attackBtnCy = 0f
    private var dodgeBtnCx  = 0f; private var dodgeBtnCy  = 0f
    private var camBtnCx    = 0f; private var camBtnCy    = 0f

    // HUD data
    var playerHp    = 150; var playerMaxHp = 150
    var playerLevel = 1;   var playerXp    = 0;  var playerNextXp = 100

    // Paints
    private val paintBar   = Paint(Paint.ANTI_ALIAS_FLAG)
    private val paintBg    = Paint(Paint.ANTI_ALIAS_FLAG)
    private val paintText  = Paint(Paint.ANTI_ALIAS_FLAG)
    private val paintStick = Paint(Paint.ANTI_ALIAS_FLAG)
    private val paintBtn   = Paint(Paint.ANTI_ALIAS_FLAG)

    init {
        setWillNotDraw(false)
        isClickable = true

        paintBg.color   = 0x88000000.toInt()
        paintBar.color  = Color.RED
        paintText.color = Color.WHITE
        paintText.textSize = 32f
        paintStick.color= 0x88FFFFFF.toInt()
        paintBtn.color  = 0x88FF4444.toInt()
    }

    override fun onSizeChanged(w: Int, h: Int, oldW: Int, oldH: Int) {
        // Joystick: bottom-left
        stickBaseX = stickBaseRadius + 40f
        stickBaseY = h - stickBaseRadius - 40f
        stickKnobX = stickBaseX; stickKnobY = stickBaseY

        // Attack button: bottom-right
        attackBtnCx = w - btnSize - 40f
        attackBtnCy = h - btnSize - 40f
        // Dodge button: to the left of attack
        dodgeBtnCx  = w - btnSize*3 - 60f
        dodgeBtnCy  = h - btnSize - 40f
        // Camera toggle: top-right
        camBtnCx    = w - 60f
        camBtnCy    = 60f
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()

        // HP bar
        val barW = w * 0.3f
        val barH = 24f
        val barX = w*0.5f - barW*0.5f
        val barY = 30f
        paintBg.color = 0x88000000.toInt()
        canvas.drawRoundRect(RectF(barX,barY,barX+barW,barY+barH), 12f,12f, paintBg)
        paintBar.color = Color.RED
        val hpRatio = playerHp.toFloat() / playerMaxHp.coerceAtLeast(1)
        canvas.drawRoundRect(RectF(barX,barY,barX+barW*hpRatio,barY+barH), 12f,12f, paintBar)
        paintText.textSize = 22f
        canvas.drawText("HP $playerHp/$playerMaxHp", barX+8, barY+17f, paintText)

        // XP bar
        val xpY = barY + barH + 6f
        val xpH = 12f
        paintBg.color = 0x66000000.toInt()
        canvas.drawRoundRect(RectF(barX,xpY,barX+barW,xpY+xpH), 6f,6f, paintBg)
        paintBar.color = 0xFF44AAFF.toInt()
        val xpRatio = playerXp.toFloat() / playerNextXp.coerceAtLeast(1)
        canvas.drawRoundRect(RectF(barX,xpY,barX+barW*xpRatio,xpY+xpH), 6f,6f, paintBar)
        paintText.textSize = 18f
        canvas.drawText("Lv $playerLevel", barX+8, xpY+10f, paintText)

        // Joystick
        paintStick.alpha = 80
        canvas.drawCircle(stickBaseX, stickBaseY, stickBaseRadius, paintStick)
        paintStick.alpha = 160
        canvas.drawCircle(stickKnobX, stickKnobY, stickKnobRadius, paintStick)

        // Buttons
        paintBtn.color  = 0xAAFF3333.toInt()
        canvas.drawCircle(attackBtnCx, attackBtnCy, btnSize, paintBtn)
        paintText.textSize = 22f
        canvas.drawText("ATK", attackBtnCx-26f, attackBtnCy+8f, paintText)

        paintBtn.color = 0xAA3388FF.toInt()
        canvas.drawCircle(dodgeBtnCx, dodgeBtnCy, btnSize, paintBtn)
        canvas.drawText("DODGE", dodgeBtnCx-38f, dodgeBtnCy+8f, paintText)

        // Camera toggle button
        paintBtn.color = 0x88FFFFFF.toInt()
        canvas.drawCircle(camBtnCx, camBtnCy, 40f, paintBtn)
        paintText.color = Color.BLACK
        canvas.drawText("CAM", camBtnCx-24f, camBtnCy+8f, paintText)
        paintText.color = Color.WHITE
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        val idx    = event.actionIndex
        val pid    = event.getPointerId(idx)
        val ex     = event.getX(idx)
        val ey     = event.getY(idx)

        when(event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                // Joystick zone (left quarter)
                if(ex < width * 0.45f && ey > height * 0.5f && stickPointerId == -1) {
                    stickPointerId = pid; stickActive = true
                    stickBaseX = ex; stickBaseY = ey
                    stickKnobX = ex; stickKnobY = ey
                    invalidate(); return true
                }
                // Attack button
                if(hypot(ex-attackBtnCx, ey-attackBtnCy) < btnSize+20f) {
                    gameView.queueEvent { GameBridge.nativeSetAttack(true) }
                    postDelayed({ gameView.queueEvent { GameBridge.nativeSetAttack(false) } }, 150)
                    return true
                }
                // Dodge button
                if(hypot(ex-dodgeBtnCx, ey-dodgeBtnCy) < btnSize+20f) {
                    gameView.queueEvent { GameBridge.nativeSetDodge(true) }
                    postDelayed({ gameView.queueEvent { GameBridge.nativeSetDodge(false) } }, 150)
                    return true
                }
                // Camera toggle
                if(hypot(ex-camBtnCx, ey-camBtnCy) < 50f) {
                    gameView.queueEvent { GameBridge.nativeToggleCamera() }
                    return true
                }
                // Pass to gameView for camera orbit
                return false
            }
            MotionEvent.ACTION_MOVE -> {
                for(i in 0 until event.pointerCount) {
                    if(event.getPointerId(i) == stickPointerId) {
                        val mx = event.getX(i); val my = event.getY(i)
                        val dx = mx - stickBaseX; val dy = my - stickBaseY
                        val dist = hypot(dx, dy)
                        val clamp = min(dist, stickBaseRadius)
                        val nx = if(dist>0) dx/dist else 0f
                        val ny = if(dist>0) dy/dist else 0f
                        stickKnobX = stickBaseX + nx*clamp
                        stickKnobY = stickBaseY + ny*clamp
                        // Normalize to -1..1 and send to C++
                        val sx = (dx/stickBaseRadius).coerceIn(-1f,1f)
                        val sy = (dy/stickBaseRadius).coerceIn(-1f,1f)
                        // sy maps screen-down → positive Z (toward camera → backward)
                        gameView.queueEvent { GameBridge.nativeSetStick(sx, -sy) }
                        invalidate()
                        return true
                    }
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> {
                if(pid == stickPointerId) {
                    stickActive = false; stickPointerId = -1
                    stickKnobX = stickBaseX; stickKnobY = stickBaseY
                    gameView.queueEvent { GameBridge.nativeSetStick(0f,0f) }
                    invalidate(); return true
                }
            }
        }
        return false
    }
}
