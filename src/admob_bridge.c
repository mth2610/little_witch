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
#include <stdbool.h>

// Lưu giữ con trỏ môi trường JVM và class JNI toàn cục
JavaVM* g_vm = NULL;
jclass g_adMobBridgeClazz = NULL;

// Callback JNI_OnLoad tự động chạy khi Java loadLibrary("littlewitch")
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    JNIEnv* env = NULL;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        // Tìm class AdMobBridge trên luồng Java lúc khởi tạo và lưu làm tham chiếu toàn cục
        jclass clazz = (*env)->FindClass(env, "com/littlewitchadventure/AdMobBridge");
        if (clazz) {
            g_adMobBridgeClazz = (jclass)(*env)->NewGlobalRef(env, clazz);
            (*env)->DeleteLocalRef(env, clazz);
        }
    }
    return JNI_VERSION_1_6;
}

// Java gọi ngược về khi xem xong quảng cáo thành công
JNIEXPORT void JNICALL
Java_com_littlewitchadventure_AdMobBridge_onRewardedAdCompleted(JNIEnv *env, jobject obj) {
    s_adState = AD_STATE_REWARDED;
}

// Java gọi ngược về báo quảng cáo lỗi / tắt
JNIEXPORT void JNICALL
Java_com_littlewitchadventure_AdMobBridge_onRewardedAdFailed(JNIEnv *env, jobject obj) {
    s_adState = AD_STATE_FAILED;
}

// Hàm bổ trợ gọi Static Void Method của Java
static void callStaticVoidMethod(const char* methodName) {
    if (!g_vm || !g_adMobBridgeClazz) return;
    JNIEnv* env = NULL;
    jint res = (*g_vm)->GetEnv(g_vm, (void**)&env, JNI_VERSION_1_6);
    bool isAttached = false;
    
    // Nếu chưa Attach Thread (ví dụ chạy từ Game Loop Thread của C), ta phải Attach
    if (res == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) == JNI_OK) {
            isAttached = true;
        }
    }
    if (!env) return;
    
    jmethodID method = (*env)->GetStaticMethodID(env, g_adMobBridgeClazz, methodName, "()V");
    if (method) {
        (*env)->CallStaticVoidMethod(env, g_adMobBridgeClazz, method);
    }
    
    if (isAttached) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}
#endif

// ----------------------------------------------------------------
// HÀM CÔNG KHAI (Chạy trên PC sẽ chạy chế độ Mockup giả lập)
// ----------------------------------------------------------------

// Khởi tạo hệ thống AdMob
void AdMob_Init(void) {
    s_adState = AD_STATE_IDLE;
    
#ifdef __ANDROID__
    // MainActivity.java đã tự khởi động AdMobBridge.initialize(this) lúc onCreate(), 
    // do đó ở đây không cần phải gọi qua JNI nữa.
#else
    printf("[AdMob PC Mock] AdMob da duoc khoi tao.\n");
    AdMob_LoadRewardedAd(); // Tự động load trước trên PC
#endif
}

// Nạp sẵn quảng cáo video nhận thưởng
void AdMob_LoadRewardedAd(void) {
    s_adState = AD_STATE_LOADING;
    
#ifdef __ANDROID__
    callStaticVoidMethod("loadRewardedAd");
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
    callStaticVoidMethod("showRewardedAd");
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
