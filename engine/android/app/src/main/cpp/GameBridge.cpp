#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <memory>

#include "engine/Renderer.h"
#include "engine/Camera.h"
#include "game/World.h"

#define TAG  "GameBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)

// ---- Singletons ----
static std::unique_ptr<Renderer> gRenderer;
static std::unique_ptr<World>    gWorld;
static InputState                gInput;
static bool                      gReady = false;

extern "C" {

// Called once from GLSurfaceView.Renderer.onSurfaceCreated
JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeInit(
        JNIEnv* env, jclass, jobject assetManager, jint w, jint h) {
    LOGI("nativeInit %dx%d", w, h);
    gRenderer = std::make_unique<Renderer>();
    gWorld    = std::make_unique<World>();

    AAssetManager* am = AAssetManager_fromJava(env, assetManager);
    if(!gRenderer->init(w, h)) { LOGE("Renderer init failed"); return; }
    if(!gWorld->init(am))      { LOGE("World init failed");    return; }
    gReady = true;
    LOGI("Game ready");
}

// Called on GLSurfaceView.Renderer.onSurfaceChanged
JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeResize(
        JNIEnv*, jclass, jint w, jint h) {
    if(gRenderer) gRenderer->resize(w, h);
    if(gWorld)    gWorld->camera.aspect = (float)w/(float)h;
}

// Called every frame from GLSurfaceView.Renderer.onDrawFrame
JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeFrame(
        JNIEnv*, jclass, jfloat dt) {
    if(!gReady || !gWorld || !gRenderer) return;
    gWorld->update(dt, gInput);
    gWorld->render(*gRenderer);
}

// Input: virtual joystick
JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeSetStick(
        JNIEnv*, jclass, jfloat x, jfloat y) {
    gInput.moveStick = {x, y};
}

// Input: attack button
JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeSetAttack(
        JNIEnv*, jclass, jboolean pressed) {
    gInput.attackBtn = (bool)pressed;
}

// Input: dodge button
JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeSetDodge(
        JNIEnv*, jclass, jboolean pressed) {
    gInput.dodgeBtn = (bool)pressed;
}

// Camera rotation from touch drag
JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeCameraOrbit(
        JNIEnv*, jclass, jfloat dYaw, jfloat dPitch) {
    if(gWorld) gWorld->camera.orbit(dYaw, dPitch);
    gInput.cameraYaw = gWorld ? gWorld->camera.yaw : 0.f;
}

JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeCameraZoom(
        JNIEnv*, jclass, jfloat delta) {
    if(gWorld) gWorld->camera.zoom(delta);
}

JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeToggleCamera(
        JNIEnv*, jclass) {
    if(gWorld) gWorld->camera.toggleMode();
}

// Query player state (called from Kotlin to update HUD)
JNIEXPORT jintArray JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeGetPlayerState(
        JNIEnv* env, jclass) {
    jintArray arr = env->NewIntArray(6);
    if(!gWorld) return arr;
    jint buf[6] = {
        gWorld->localPlayer.stats.hp,
        gWorld->localPlayer.stats.maxHp,
        gWorld->localPlayer.stats.level,
        gWorld->localPlayer.xp,
        gWorld->localPlayer.nextLevelXp,
        (int)gWorld->enemies.size()
    };
    env->SetIntArrayRegion(arr, 0, 6, buf);
    return arr;
}

// Network: update remote player
JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeNetPlayerUpdate(
        JNIEnv* env, jclass,
        jint id, jstring jname, jfloat x, jfloat y, jfloat z,
        jfloat yaw, jint hp, jint maxHp, jint anim, jfloat animTime) {
    if(!gWorld) return;
    const char* cname = env->GetStringUTFChars(jname, nullptr);
    RemotePlayer rp;
    rp.id       = (uint32_t)id;
    rp.name     = cname;
    rp.position = {x,y,z};
    rp.yaw      = yaw;
    rp.hp       = hp;
    rp.maxHp    = maxHp;
    rp.animIndex= anim;
    rp.animTime = animTime;
    env->ReleaseStringUTFChars(jname, cname);
    gWorld->onNetworkPlayerUpdate(rp);
}

JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeNetPlayerLeave(
        JNIEnv*, jclass, jint id) {
    if(gWorld) gWorld->onNetworkPlayerLeave((uint32_t)id);
}

// Get local player position for network broadcast
JNIEXPORT jfloatArray JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeGetLocalPos(
        JNIEnv* env, jclass) {
    jfloatArray arr = env->NewFloatArray(5);
    if(!gWorld) return arr;
    jfloat buf[5] = {
        gWorld->localPlayer.position.x,
        gWorld->localPlayer.position.y,
        gWorld->localPlayer.position.z,
        gWorld->localPlayer.yaw,
        (float)gWorld->localPlayer.currentAnim
    };
    env->SetFloatArrayRegion(arr, 0, 5, buf);
    return arr;
}

JNIEXPORT void JNICALL
Java_com_forwardfirst_mmorpg_GameBridge_nativeDestroy(
        JNIEnv*, jclass) {
    gReady = false;
    if(gWorld)    { gWorld->destroy();    gWorld.reset();    }
    if(gRenderer) { gRenderer->destroy(); gRenderer.reset(); }
    LOGI("Game destroyed");
}

} // extern "C"
