package com.forwardfirst.mmorpg

import android.content.res.AssetManager

object GameBridge {
    init { System.loadLibrary("mmorpg") }

    @JvmStatic external fun nativeInit(assetManager: AssetManager, w: Int, h: Int)
    @JvmStatic external fun nativeResize(w: Int, h: Int)
    @JvmStatic external fun nativeFrame(dt: Float)
    @JvmStatic external fun nativeSetStick(x: Float, y: Float)
    @JvmStatic external fun nativeSetAttack(pressed: Boolean)
    @JvmStatic external fun nativeSetDodge(pressed: Boolean)
    @JvmStatic external fun nativeCameraOrbit(dYaw: Float, dPitch: Float)
    @JvmStatic external fun nativeCameraZoom(delta: Float)
    @JvmStatic external fun nativeToggleCamera()
    @JvmStatic external fun nativeGetPlayerState(): IntArray
    @JvmStatic external fun nativeGetLocalPos(): FloatArray
    @JvmStatic external fun nativeNetPlayerUpdate(
        id: Int, name: String, x: Float, y: Float, z: Float,
        yaw: Float, hp: Int, maxHp: Int, anim: Int, animTime: Float
    )
    @JvmStatic external fun nativeNetPlayerLeave(id: Int)
    @JvmStatic external fun nativeDestroy()
}
