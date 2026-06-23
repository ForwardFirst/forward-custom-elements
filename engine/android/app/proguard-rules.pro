# Keep JNI bridge
-keep class com.forwardfirst.mmorpg.GameBridge { *; }

# OkHttp
-dontwarn okhttp3.**
-dontwarn okio.**
-keep class okhttp3.** { *; }
