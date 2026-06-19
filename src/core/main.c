// ============================================================
// main.c — Entry Point & Vòng lặp Game chính
// ============================================================

#include "admob_bridge.h"
#include "biome.h"
#include "config.h"
#include "enemy.h"
#include "game.h"
#include "gameplay.h"
#include "lightmap.h"
#include "particle.h"
#include "raylib.h"
#include "resource_loader.h"
#include "save.h"
#include "skill.h"
#include "star.h"
#include "trail.h"
#include "ui.h"
#include "witch.h"
#include "screen.h"
#include <stdlib.h>
#include <time.h>

// ----------------------------------------------------------------
// CHƯƠNG TRÌNH CHÍNH (main)
// ----------------------------------------------------------------
int main(void) {
#ifndef __ANDROID__
  // Tự động tìm và chuyển đổi thư mục làm việc về thư mục gốc chứa tài nguyên
  // nếu chạy từ thư mục con (ví dụ: build/ hay build/bin/)
  if (!DirectoryExists("assets")) {
    if (DirectoryExists("../assets")) {
      ChangeDirectory("..");
    } else if (DirectoryExists("../../assets")) {
      ChangeDirectory("../..");
    }
  }
#endif

  // Khởi tạo hạt giống ngẫu nhiên thời gian thực
  srand((unsigned int)time(NULL));

  // Khởi tạo Raylib Window
#ifdef __ANDROID__
  SetConfigFlags(FLAG_FULLSCREEN_MODE);
  InitWindow(0, 0, "Little Witch Adventure");
#else
  InitWindow(1280, 720, "Little Witch Adventure");
#endif

  SetTargetFPS(TARGET_FPS);
  InitAudioDevice();

  // Nạp gói dữ liệu lưu trữ
  SaveData save = LoadSaveData();

  // Khởi tạo cấu trúc GameState toàn cục
  GameState gs = {0};
  gs.currentScreen = SCREEN_MENU;
  gs.currentBiome = BIOME_FOREST;
  gs.savedUpgrades = save.upgrades;
  gs.language = save.language;
  gs.cachedSave = save;
  gs.biomeStartScore = 0;
  gs.bossSpawned = false;
  gs.shakeDuration = 0.0f;
  gs.shakeIntensity = 0.0f;

  // Khởi tạo hệ thống quảng cáo AdMob
  AdMob_Init();

  // Nạp tất cả tài nguyên game (textures, sounds, shaders, fonts, render textures)
  LoadGameResources(&gs);
  InitScreenManager(&gs);

  // ----------------------------------------------------------------
  // GAME LOOP CHÍNH
  // ----------------------------------------------------------------
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // --- 1. CẬP NHẬT LOGIC THEO SCREEN HIỆN TẠI ---
    UpdateCurrentScreen(&gs, dt);

    // --- 2. RENDER VÀO VIRTUAL CANVAS 1280x720 ---
    BeginTextureMode(gs.virtualCanvas);
    ClearBackground(BLACK);
    DrawCurrentScreen(&gs, dt);
    EndTextureMode();

    // --- 3. SCALE VÀ VẼ VIRTUAL CANVAS RA MÀN HÌNH THẬT (LETTERBOXING) ---
    DrawVirtualCanvasToScreen(&gs);
  }

  // ----------------------------------------------------------------
  // GIẢI PHÓNG BỘ NHỚ VÀ TÀI NGUYÊN (CLEANUP)
  // ----------------------------------------------------------------
  UnloadScreenManager(&gs);
  UnloadGameResources(&gs);

  CloseAudioDevice();
  CloseWindow();

  return 0;
}
