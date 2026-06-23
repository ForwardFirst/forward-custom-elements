package com.forwardfirst.mmorpg

import android.os.Handler
import android.os.Looper
import okhttp3.*
import org.json.JSONObject
import java.util.concurrent.TimeUnit

// Packet type constants (must match server)
object Packet {
    const val PLAYER_JOIN   = "player_join"
    const val PLAYER_UPDATE = "player_update"
    const val PLAYER_LEAVE  = "player_leave"
    const val ENEMY_UPDATE  = "enemy_update"
    const val CHAT          = "chat"
    const val PING          = "ping"
    const val PONG          = "pong"
    const val AUTH          = "auth"
}

class NetworkManager(private val hudView: HUDView, private val gameView: GameSurfaceView) {

    private val client  = OkHttpClient.Builder()
        .pingInterval(30, TimeUnit.SECONDS)
        .build()
    private var ws: WebSocket? = null
    private val mainHandler = Handler(Looper.getMainLooper())

    var localPlayerId = -1
    var localPlayerName = "Warrior"
    private var sendInterval  = 0
    private val SEND_RATE     = 5   // send every 5 frames (~12Hz at 60fps)
    var connected = false

    fun connect(serverUrl: String, playerName: String) {
        localPlayerName = playerName
        val request = Request.Builder().url(serverUrl).build()
        ws = client.newWebSocket(request, object : WebSocketListener() {

            override fun onOpen(webSocket: WebSocket, response: Response) {
                connected = true
                // Authenticate
                val auth = JSONObject().apply {
                    put("type", Packet.AUTH)
                    put("name", playerName)
                }
                webSocket.send(auth.toString())
            }

            override fun onMessage(webSocket: WebSocket, text: String) {
                try { handleMessage(JSONObject(text)) }
                catch(e: Exception) { /* ignore malformed */ }
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                connected = false
                // Retry after 5s
                mainHandler.postDelayed({ connect(serverUrl, playerName) }, 5000)
            }

            override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                connected = false
            }
        })
    }

    private fun handleMessage(json: JSONObject) {
        when(json.optString("type")) {
            Packet.AUTH -> {
                localPlayerId = json.optInt("id", -1)
            }
            Packet.PLAYER_JOIN, Packet.PLAYER_UPDATE -> {
                val id   = json.optInt("id")
                if(id == localPlayerId) return
                val name = json.optString("name","Unknown")
                val x    = json.optDouble("x").toFloat()
                val y    = json.optDouble("y").toFloat()
                val z    = json.optDouble("z").toFloat()
                val yaw  = json.optDouble("yaw").toFloat()
                val hp   = json.optInt("hp")
                val maxHp= json.optInt("maxHp")
                val anim = json.optInt("anim")
                val at   = json.optDouble("animTime").toFloat()
                gameView.queueEvent {
                    GameBridge.nativeNetPlayerUpdate(id,name,x,y,z,yaw,hp,maxHp,anim,at)
                }
            }
            Packet.PLAYER_LEAVE -> {
                val id = json.optInt("id")
                gameView.queueEvent { GameBridge.nativeNetPlayerLeave(id) }
            }
            Packet.PING -> {
                ws?.send(JSONObject().apply { put("type", Packet.PONG) }.toString())
            }
        }
    }

    // Called from game loop (on GL thread) to broadcast local player state
    fun onFrame() {
        if(!connected || ws==null) return
        sendInterval++
        if(sendInterval < SEND_RATE) return
        sendInterval = 0

        val pos = GameBridge.nativeGetLocalPos()
        val state = GameBridge.nativeGetPlayerState()

        val json = JSONObject().apply {
            put("type",     Packet.PLAYER_UPDATE)
            put("id",       localPlayerId)
            put("name",     localPlayerName)
            put("x",        pos[0]); put("y", pos[1]); put("z", pos[2])
            put("yaw",      pos[3])
            put("hp",       state[0]); put("maxHp", state[1])
            put("anim",     pos[4].toInt())
            put("animTime", 0f)
        }
        ws?.send(json.toString())
    }

    fun disconnect() { ws?.close(1000,"bye"); connected=false }
}
