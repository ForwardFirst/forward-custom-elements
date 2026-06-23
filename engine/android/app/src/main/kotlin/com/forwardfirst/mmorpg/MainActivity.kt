package com.forwardfirst.mmorpg

import android.app.Activity
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.WindowManager
import android.widget.FrameLayout

class MainActivity : Activity() {

    private lateinit var gameView:  GameSurfaceView
    private lateinit var hudView:   HUDView
    private lateinit var network:   NetworkManager
    private val handler  = Handler(Looper.getMainLooper())
    private val HUD_TICK = 200L   // ms between HUD refreshes

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        window.decorView.systemUiVisibility = (
            View.SYSTEM_UI_FLAG_FULLSCREEN or
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        )

        gameView = GameSurfaceView(this)
        hudView  = HUDView(this, gameView)
        network  = NetworkManager(hudView, gameView)

        val root = FrameLayout(this)
        root.addView(gameView)
        root.addView(hudView)
        setContentView(root)

        // Connect to game server (change IP for production)
        network.connect("ws://10.0.2.2:3000", "Warrior_${(1000..9999).random()}")

        scheduleHudUpdate()
    }

    private fun scheduleHudUpdate() {
        handler.postDelayed({
            if(!isDestroyed) {
                val state = GameBridge.nativeGetPlayerState()
                hudView.playerHp     = state[0]
                hudView.playerMaxHp  = state[1]
                hudView.playerLevel  = state[2]
                hudView.playerXp     = state[3]
                hudView.playerNextXp = state[4]
                hudView.invalidate()
                network.onFrame()
                scheduleHudUpdate()
            }
        }, HUD_TICK)
    }

    override fun onResume() {
        super.onResume()
        gameView.onResume()
    }

    override fun onPause() {
        super.onPause()
        gameView.onPause()
    }

    override fun onDestroy() {
        handler.removeCallbacksAndMessages(null)
        network.disconnect()
        gameView.queueEvent { GameBridge.nativeDestroy() }
        super.onDestroy()
    }
}
