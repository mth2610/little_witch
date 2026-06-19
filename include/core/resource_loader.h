#ifndef RESOURCE_LOADER_H
#define RESOURCE_LOADER_H

#include "raylib.h"

// Forward declaration
struct GameState;

// ----------------------------------------------------------------
// BIẾN TOÀN CỤC CHỨA TÀI NGUYÊN (extern declarations)
// ----------------------------------------------------------------

// Texture phù thủy
extern Texture2D witchFlyTexture;
extern Texture2D witchAttackHandTexture;
extern Texture2D witchAttackWeaponTexture;
extern Texture2D witchDefenseTexture;
extern Texture2D witchSpritesheetTexture;
extern bool witchSpritesheetLoaded;
extern Texture2D fluteTexture;

// Texture ngôi sao
extern Texture2D starNormalTexture;
extern Texture2D starGoldTexture;
extern Texture2D starRainbowTexture;

// Texture quái vật thường
extern Texture2D enemySlimeTexture;
extern Texture2D enemyBatTexture;
extern Texture2D enemyGhostTexture;
extern Texture2D enemyRobotTexture;
extern Texture2D enemyDroneTexture;
extern Texture2D enemyAlienTexture;
extern Texture2D enemyUfoTexture;

// Texture boss
extern Texture2D bossForestTexture;
extern Texture2D bossCaveTexture;
extern Texture2D bossCityTexture;
extern Texture2D bossSpaceTexture;

// Texture nền biome
extern Texture2D bgForestTexture;
extern Texture2D bgCaveTexture;
extern Texture2D bgCityTexture;
extern Texture2D bgSpaceTexture;

// Âm thanh
extern Sound collectStarSound;
extern Sound hitEnemySound;
extern Sound gameOverSound;
extern Sound shootSound;
extern Music bgMusic;

extern Texture2D shieldBackgroundTexture;

// Nạp tất cả tài nguyên game (textures, sounds, shaders, fonts, render textures)
void LoadGameResources(struct GameState *gs);

// Giải phóng tất cả tài nguyên game
void UnloadGameResources(struct GameState *gs);

#endif // RESOURCE_LOADER_H
