package com.littlewitchadventure;

import com.google.android.gms.ads.*;
import com.google.android.gms.ads.rewarded.*;

public class AdMobBridge {
    private static RewardedAd rewardedAd = null;
    private static MainActivity activity;

    // Ad Unit ID Test cho QC video nhận thưởng
    private static final String REWARDED_AD_UNIT_ID = "ca-app-pub-3940256099942544/5224354917";

    public static void initialize(MainActivity act) {
        activity = act;
        // Chạy trên luồng UI của Android
        activity.runOnUiThread(() -> {
            MobileAds.initialize(activity, initializationStatus -> {});
            loadRewardedAd();
        });
    }

    public static void loadRewardedAd() {
        activity.runOnUiThread(() -> {
            AdRequest adRequest = new AdRequest.Builder().build();
            RewardedAd.load(activity, REWARDED_AD_UNIT_ID, adRequest,
                new RewardedAdLoadCallback() {
                    @Override
                    public void onAdLoaded(RewardedAd ad) {
                        rewardedAd = ad;
                    }
                    @Override
                    public void onAdFailedToLoad(LoadAdError error) {
                        rewardedAd = null;
                        onRewardedAdFailed(); // Callback ngược về C báo lỗi
                    }
                });
        });
    }

    public static void showRewardedAd() {
        activity.runOnUiThread(() -> {
            if (rewardedAd != null) {
                rewardedAd.show(activity, rewardItem -> {
                    // Chạy callback C báo xem thành công nhận thưởng
                    onRewardedAdCompleted();
                });
            } else {
                onRewardedAdFailed();
                loadRewardedAd(); // Thử nạp lại
            }
        });
    }

    // Khai báo các hàm native kết nối C
    private static native void onRewardedAdCompleted();
    private static native void onRewardedAdFailed();

    // Load thư viện C tĩnh của game
    static { System.loadLibrary("littlewitch"); }
}
