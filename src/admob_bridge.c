// ============================================================
// admob_bridge.c — Cầu nối gọi Quảng cáo AdMob (JNI Bridge)
// ============================================================

#include "admob_bridge.h"
#include <stdio.h>

// Trạng thái quảng cáo nội bộ
static AdState s_adState = AD_STATE_IDLE;

// ----------------------------------------------------------------
// [CHỈ DÀNH CHO ANDROID] Giao tiếp JNI thực tế
// ----------------------------------------------------------------
#ifdef __ANDROID__
#include <jni.h>

// Con trỏ môi trường JVM và Class Java dùng để gọi ngược lại từ C sang Java
extern JavaVM* g_vm;
extern jobject g_adMobBridgeObj;

JNIEXPORT void JNICALL
Java_com_littlewitchadventure_AdMobBridge_onRewardedAdCompleted(JNIEnv *env, jobject obj) {
    s_adState = AD_STATE_REWARDED;
}

JNIEXPORT void JNICALL
Java_com_littlewitchadventure_AdMobBridge_onRewardedAdFailed(JNIEnv *env, jobject obj) {
    s_adState = AD_STATE_FAILED;
}
#endif

// ----------------------------------------------------------------
// HÀM CÔNG KHAI (Chạy trên PC sẽ chạy chế độ Mockup giả lập)
// ----------------------------------------------------------------

// Khởi tạo hệ thống AdMob
void AdMob_Init(void) {
    s_adState = AD_STATE_IDLE;
    
#ifdef __ANDROID__
    // Gọi phương thức initialize() của class Java thông qua JNI
    // (JNI setup phức tạp sẽ được triển khai chi tiết khi build Android)
    // Tạm thời s_adState = AD_STATE_IDLE;
#else
    printf("[AdMob PC Mock] AdMob da duoc khoi tao.\n");
    AdMob_LoadRewardedAd(); // Tự động load trước trên PC
#endif
}

// Nạp sẵn quảng cáo video nhận thưởng
void AdMob_LoadRewardedAd(void) {
    s_adState = AD_STATE_LOADING;
    
#ifdef __ANDROID__
    // Gọi phương thức loadRewardedAd() của Java qua JNI
#else
    // Giả lập nạp quảng cáo thành công ngay lập tức trên PC
    s_adState = AD_STATE_READY;
    printf("[AdMob PC Mock] Quang cao da san sang.\n");
#endif
}

// Trình chiếu quảng cáo
void AdMob_ShowRewardedAd(void) {
    s_adState = AD_STATE_SHOWING;
    
#ifdef __ANDROID__
    // Gọi phương thức showRewardedAd() của Java qua JNI
#else
    // Giả lập trình chiếu quảng cáo thành công và nhận thưởng sau 1 frame trên PC
    s_adState = AD_STATE_REWARDED;
    printf("[AdMob PC Mock] Quang cao da chieu xong. Da nhan thuong!\n");
#endif
}

// Lấy trạng thái quảng cáo hiện tại để game loop xử lý hồi sinh
AdState AdMob_GetState(void) {
    return s_adState;
}

// Đưa trạng thái quảng cáo về rảnh rỗi (IDLE)
void AdMob_ResetState(void) {
    s_adState = AD_STATE_IDLE;
}
