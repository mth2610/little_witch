#ifndef CONFIG_H
#define CONFIG_H

#include "raylib.h"

// ============================================================
// config.h — Hằng số và cấu hình toàn cục
// ============================================================

// Hệ thống màn hình
#define VIRTUAL_WIDTH           1280
#define VIRTUAL_HEIGHT          720
#define TARGET_FPS              60

// Object Pool limits
#define MAX_ENEMIES             32
#define MAX_STARS               8
#define MAX_PARTICLES           128
#define MAX_PROJECTILES         48

// Gameplay tuning
#define PLAYER_BASE_SPEED       300.0f
#define STAR_ORBIT_RADIUS       85.0f
#define STAR_ORBIT_SPEED        0.9f
#define STAR_COLLISION_RADIUS   18.0f
#define BOSS_SPAWN_SCORE        500
#define REVIVE_GOLD_COST        0
#define SWIPE_THRESHOLD         30.0f

// Trạng thái màn hình
typedef enum GameScreen {
    SCREEN_MENU = 0,
    SCREEN_GAMEPLAY,
    SCREEN_SKILL_SELECT,
    SCREEN_UPGRADE_SHOP,
    SCREEN_GAMEOVER
} GameScreen;

// Trạng thái Vùng đất
typedef enum BiomeState {
    BIOME_FOREST = 0,
    BIOME_CAVE,
    BIOME_CITY,
    BIOME_SPACE
} BiomeState;

// Trạng thái AdMob (dùng cờ để sync C ↔ Java)
typedef enum AdState {
    AD_STATE_IDLE = 0,
    AD_STATE_LOADING,
    AD_STATE_READY,
    AD_STATE_SHOWING,
    AD_STATE_REWARDED,
    AD_STATE_FAILED
} AdState;

// Hệ thống ngôn ngữ hỗ trợ tiếng Việt và tiếng Anh
typedef enum Language {
    LANG_VI = 0,
    LANG_EN = 1
} Language;

// Định nghĩa thêm màu CYAN không có sẵn trong Raylib
#ifndef CYAN
#define CYAN CLITERAL(Color){ 0, 255, 255, 255 }
#endif

// Thư viện Raylib 5.5 đổi tên các hàm IsXReady thành IsXValid.
// Định nghĩa các macro tương thích nếu biên dịch với phiên bản Raylib >= 5.5.
#if defined(RAYLIB_VERSION_MAJOR) && (RAYLIB_VERSION_MAJOR >= 5) && defined(RAYLIB_VERSION_MINOR) && (RAYLIB_VERSION_MINOR >= 5)
  #define IsTextureReady(x)       IsTextureValid(x)
  #define IsSoundReady(x)         IsSoundValid(x)
  #define IsMusicReady(x)         IsMusicValid(x)
  #define IsShaderReady(x)        IsShaderValid(x)
  #define IsFontReady(x)          IsFontValid(x)
  #define IsRenderTextureReady(x) IsRenderTextureValid(x)
#endif

// Hỗ trợ Android: tự động loại bỏ tiền tố "assets/" vì tài nguyên trong APK 
// được quản lý trực tiếp tại thư mục gốc của AssetManager.
#ifdef __ANDROID__
  #define SHADER_PATH(name) "assets/shaders/gles/" name
#else
  #define SHADER_PATH(name) "assets/shaders/glsl330/" name
#endif

#ifdef __ANDROID__
  #include <string.h>
  static inline const char* stripAssetsPrefix(const char* path) {
      if (path && strncmp(path, "assets/", 7) == 0) {
          return path + 7;
      }
      return path;
  }
  #define FileExists(path)          FileExists(stripAssetsPrefix(path))
  #define LoadTexture(path)         LoadTexture(stripAssetsPrefix(path))
  #define LoadShader(vs, fs)        LoadShader(stripAssetsPrefix(vs), stripAssetsPrefix(fs))
  #define LoadFontEx(path, s, c, n)  LoadFontEx(stripAssetsPrefix(path), s, c, n)
  #define LoadSound(path)           LoadSound(stripAssetsPrefix(path))
  #define LoadMusicStream(path)     LoadMusicStream(stripAssetsPrefix(path))
#endif

#endif // CONFIG_H
