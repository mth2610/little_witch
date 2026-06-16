#ifndef CONFIG_H
#define CONFIG_H

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

#endif // CONFIG_H
