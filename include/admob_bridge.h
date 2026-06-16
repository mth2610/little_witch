#ifndef ADMOB_BRIDGE_H
#define ADMOB_BRIDGE_H

#include "config.h"

// Khởi tạo AdMob (gọi 1 lần khi khởi động)
void AdMob_Init(void);

// Nạp sẵn Rewarded Ad (gọi ngay sau khi game bắt đầu)
void AdMob_LoadRewardedAd(void);

// Hiển thị Rewarded Ad
// Sau khi xem xong → Java gọi AdMob_OnRewarded() callback
void AdMob_ShowRewardedAd(void);

// Lấy trạng thái hiện tại (poll từ game loop)
AdState AdMob_GetState(void);

// Reset về IDLE sau khi game loop đã xử lý kết quả
void AdMob_ResetState(void);

// [CHỈ ANDROID] Callback được gọi TỪ Java khi người dùng xem xong Ad hoặc Ad bị lỗi
#ifdef __ANDROID__
#include <jni.h>
JNIEXPORT void JNICALL
Java_com_littlewitchadventure_AdMobBridge_onRewardedAdCompleted(JNIEnv *env, jobject obj);

JNIEXPORT void JNICALL
Java_com_littlewitchadventure_AdMobBridge_onRewardedAdFailed(JNIEnv *env, jobject obj);
#endif

#endif // ADMOB_BRIDGE_H
