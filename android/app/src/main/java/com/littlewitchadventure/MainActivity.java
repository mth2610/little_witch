package com.littlewitchadventure;

import android.app.NativeActivity;
import android.os.Bundle;
import android.view.WindowManager;

public class MainActivity extends NativeActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Cấu hình game giữ màn hình luôn sáng, không tắt màn hình khi đang chơi
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        // Khởi tạo quảng cáo AdMob
        AdMobBridge.initialize(this);
    }
}
