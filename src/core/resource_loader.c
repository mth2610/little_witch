// ============================================================
// resource_loader.c — Nạp và giải phóng tất cả tài nguyên game
// ============================================================

#include "resource_loader.h"
#include "config.h"
#include "game.h"
#include "skill.h"
#include "raylib.h"
#include <stddef.h>

// ----------------------------------------------------------------
// ĐỊNH NGHĨA BIẾN TOÀN CỤC CHỨA TÀI NGUYÊN
// ----------------------------------------------------------------

// Texture phù thủy
Texture2D witchFlyTexture;
Texture2D witchAttackHandTexture;
Texture2D witchAttackWeaponTexture;
Texture2D witchDefenseTexture;
Texture2D witchSpritesheetTexture;
bool witchSpritesheetLoaded = false;
Texture2D fluteTexture;

// Texture ngôi sao
Texture2D starNormalTexture;
Texture2D starGoldTexture;
Texture2D starRainbowTexture;

// Texture quái vật thường
Texture2D enemySlimeTexture;
Texture2D enemyBatTexture;
Texture2D enemyGhostTexture;
Texture2D enemyRobotTexture;
Texture2D enemyDroneTexture;
Texture2D enemyAlienTexture;
Texture2D enemyUfoTexture;

// Texture boss
Texture2D bossForestTexture;
Texture2D bossCaveTexture;
Texture2D bossCityTexture;
Texture2D bossSpaceTexture;

// Texture nền biome
Texture2D bgForestTexture;
Texture2D bgCaveTexture;
Texture2D bgCityTexture;
Texture2D bgSpaceTexture;

// Âm thanh
Sound collectStarSound;
Sound hitEnemySound;
Sound gameOverSound;
Sound shootSound;
Music bgMusic;

// Phông chữ vector toàn cục
Font gameFont;

// Shader khiên bảo vệ toàn cục
Shader shieldShader;
bool shieldShaderLoaded = false;
Texture2D shieldBackgroundTexture;

// ----------------------------------------------------------------
// NẠP TÀI NGUYÊN
// ----------------------------------------------------------------
void LoadGameResources(GameState *gs) {
  // Nạp phông chữ vector sắc nét nếu tồn tại, nếu không dùng phông mặc định của
  // Raylib
  if (FileExists("assets/fonts/clean_font.ttf")) {
    gameFont = LoadFontEx("assets/fonts/clean_font.ttf", 64, NULL, 0);
    SetTextureFilter(gameFont.texture, TEXTURE_FILTER_BILINEAR);
  } else {
    gameFont = GetFontDefault();
  }

  // Khởi tạo Virtual Canvas kích thước cố định 1280x720
  gs->virtualCanvas = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
  SetTextureFilter(gs->virtualCanvas.texture, TEXTURE_FILTER_BILINEAR);

  // Khởi tạo Lightmap Canvas kích thước cố định 1280x720
  gs->lightmap = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
  SetTextureFilter(gs->lightmap.texture, TEXTURE_FILTER_BILINEAR);

  // Khởi tạo Screen Copy Canvas kích thước cố định 1280x720 cho hiệu ứng
  // Distortion
  gs->screenCopyTex = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
  SetTextureFilter(gs->screenCopyTex.texture, TEXTURE_FILTER_BILINEAR);

  // Khởi tạo texture ảo 2x2 màu trắng để map UV cho shader
  Image dummyImg = GenImageColor(2, 2, WHITE);
  gs->dummyWhiteTex = LoadTextureFromImage(dummyImg);
  UnloadImage(dummyImg);

  // Khởi tạo Bloom Canvases (320x180 để tối ưu hóa hiệu năng)
  gs->bloomCanvas = LoadRenderTexture(320, 180);
  SetTextureFilter(gs->bloomCanvas.texture, TEXTURE_FILTER_BILINEAR);
  gs->blurCanvas = LoadRenderTexture(320, 180);
  SetTextureFilter(gs->blurCanvas.texture, TEXTURE_FILTER_BILINEAR);
  InitSkills();

  // Nạp Bloom Shaders cho hiệu ứng Ori-glow
  if (FileExists(SHADER_PATH("bloom_extract.fs"))) {
    gs->bloomExtractShader = LoadShader(NULL, SHADER_PATH("bloom_extract.fs"));
    gs->bloomExtractShaderLoaded = true;
  } else {
    gs->bloomExtractShaderLoaded = false;
  }
  if (FileExists(SHADER_PATH("bloom_blur.fs"))) {
    gs->bloomBlurShader = LoadShader(NULL, SHADER_PATH("bloom_blur.fs"));
    gs->bloomBlurShaderLoaded = true;
  } else {
    gs->bloomBlurShaderLoaded = false;
  }

  // Nạp Shader hạt sương giá (Frost Vapor)
  if (FileExists(SHADER_PATH("vapor.fs"))) {
    gs->vaporShader = LoadShader(NULL, SHADER_PATH("vapor.fs"));
    gs->vaporShaderLoaded = true;
  } else {
    gs->vaporShaderLoaded = false;
  }



  // ----------------------------------------------------------------
  // NẠP TÀI NGUYÊN GAME (LOAD TEXTURE & SOUNDS)
  // ----------------------------------------------------------------
  // Kiểm tra an toàn sự tồn tại của file trước khi load, tránh treo app
  if (FileExists("assets/little_witch.png")) {
    witchSpritesheetTexture = LoadTexture("assets/little_witch.png");
    witchSpritesheetLoaded = true;
  } else {
    witchSpritesheetLoaded = false;
  }
  if (FileExists("assets/textures/witch_fly.png")) {
    witchFlyTexture = LoadTexture("assets/textures/witch_fly.png");
  }
  if (FileExists("assets/textures/witch_attack_hand.png")) {
    witchAttackHandTexture =
        LoadTexture("assets/textures/witch_attack_hand.png");
  }
  if (FileExists("assets/textures/witch_attack_weapon.png")) {
    witchAttackWeaponTexture =
        LoadTexture("assets/textures/witch_attack_weapon.png");
  }
  if (FileExists("assets/textures/witch_defense.png")) {
    witchDefenseTexture = LoadTexture("assets/textures/witch_defense.png");
  }
  if (FileExists("assets/textures/star_normal.png")) {
    starNormalTexture = LoadTexture("assets/textures/star_normal.png");
  }
  if (FileExists("assets/textures/star_gold.png")) {
    starGoldTexture = LoadTexture("assets/textures/star_gold.png");
  }
  if (FileExists("assets/textures/star_rainbow.png")) {
    starRainbowTexture = LoadTexture("assets/textures/star_rainbow.png");
  }
  if (FileExists("assets/textures/flute.png")) {
    fluteTexture = LoadTexture("assets/textures/flute.png");
  }

  if (FileExists("assets/textures/enemies/slime.png")) {
    enemySlimeTexture = LoadTexture("assets/textures/enemies/slime.png");
  }
  if (FileExists("assets/textures/enemies/bat.png")) {
    enemyBatTexture = LoadTexture("assets/textures/enemies/bat.png");
  }
  if (FileExists("assets/textures/enemies/ghost.png")) {
    enemyGhostTexture = LoadTexture("assets/textures/enemies/ghost.png");
  }
  if (FileExists("assets/textures/enemies/robot.png")) {
    enemyRobotTexture = LoadTexture("assets/textures/enemies/robot.png");
  }
  if (FileExists("assets/textures/enemies/drone.png")) {
    enemyDroneTexture = LoadTexture("assets/textures/enemies/drone.png");
  }
  if (FileExists("assets/textures/enemies/alien.png")) {
    enemyAlienTexture = LoadTexture("assets/textures/enemies/alien.png");
  }
  if (FileExists("assets/textures/enemies/ufo.png")) {
    enemyUfoTexture = LoadTexture("assets/textures/enemies/ufo.png");
  }

  if (FileExists("assets/textures/enemies/boss_forest.png")) {
    bossForestTexture = LoadTexture("assets/textures/enemies/boss_forest.png");
  }
  if (FileExists("assets/textures/enemies/boss_cave.png")) {
    bossCaveTexture = LoadTexture("assets/textures/enemies/boss_cave.png");
  }
  if (FileExists("assets/textures/enemies/boss_city.png")) {
    bossCityTexture = LoadTexture("assets/textures/enemies/boss_city.png");
  }
  if (FileExists("assets/textures/enemies/boss_space.png")) {
    bossSpaceTexture = LoadTexture("assets/textures/enemies/boss_space.png");
  }

  if (FileExists("assets/textures/biomes/bg_forest.png")) {
    bgForestTexture = LoadTexture("assets/textures/biomes/bg_forest.png");
  }
  if (FileExists("assets/textures/biomes/bg_cave.png")) {
    bgCaveTexture = LoadTexture("assets/textures/biomes/bg_cave.png");
  }
  if (FileExists("assets/textures/biomes/bg_city.png")) {
    bgCityTexture = LoadTexture("assets/textures/biomes/bg_city.png");
  }
  if (FileExists("assets/textures/biomes/bg_space.png")) {
    bgSpaceTexture = LoadTexture("assets/textures/biomes/bg_space.png");
  }

  if (FileExists("assets/sounds/collect_star.wav")) {
    collectStarSound = LoadSound("assets/sounds/collect_star.wav");
  }
  if (FileExists("assets/sounds/hit_enemy.wav")) {
    hitEnemySound = LoadSound("assets/sounds/hit_enemy.wav");
  }
  if (FileExists("assets/sounds/game_over.wav")) {
    gameOverSound = LoadSound("assets/sounds/game_over.wav");
  }
  if (FileExists("assets/sounds/shoot_fireball.wav")) {
    shootSound = LoadSound("assets/sounds/shoot_fireball.wav");
  }
  if (FileExists("assets/sounds/bgm_forest.ogg")) {
    bgMusic = LoadMusicStream("assets/sounds/bgm_forest.ogg");
    bgMusic.looping = true;
    PlayMusicStream(bgMusic);
  }
}

// ----------------------------------------------------------------
// GIẢI PHÓNG TÀI NGUYÊN
// ----------------------------------------------------------------
void UnloadGameResources(GameState *gs) {
  UnloadRenderTexture(gs->virtualCanvas);
  UnloadRenderTexture(gs->lightmap);
  UnloadRenderTexture(gs->screenCopyTex);
  UnloadRenderTexture(gs->bloomCanvas);
  UnloadRenderTexture(gs->blurCanvas);
  if (IsTextureReady(gs->dummyWhiteTex))
    UnloadTexture(gs->dummyWhiteTex);

  if (IsTextureReady(witchFlyTexture))
    UnloadTexture(witchFlyTexture);
  if (IsTextureReady(witchAttackHandTexture))
    UnloadTexture(witchAttackHandTexture);
  if (IsTextureReady(witchAttackWeaponTexture))
    UnloadTexture(witchAttackWeaponTexture);
  if (IsTextureReady(witchDefenseTexture))
    UnloadTexture(witchDefenseTexture);
  if (IsTextureReady(witchSpritesheetTexture))
    UnloadTexture(witchSpritesheetTexture);
  if (IsTextureReady(starNormalTexture))
    UnloadTexture(starNormalTexture);
  if (IsTextureReady(starGoldTexture))
    UnloadTexture(starGoldTexture);
  if (IsTextureReady(starRainbowTexture))
    UnloadTexture(starRainbowTexture);
  if (IsTextureReady(fluteTexture))
    UnloadTexture(fluteTexture);

  if (IsTextureReady(enemySlimeTexture))
    UnloadTexture(enemySlimeTexture);
  if (IsTextureReady(enemyBatTexture))
    UnloadTexture(enemyBatTexture);
  if (IsTextureReady(enemyGhostTexture))
    UnloadTexture(enemyGhostTexture);
  if (IsTextureReady(enemyRobotTexture))
    UnloadTexture(enemyRobotTexture);
  if (IsTextureReady(enemyDroneTexture))
    UnloadTexture(enemyDroneTexture);
  if (IsTextureReady(enemyAlienTexture))
    UnloadTexture(enemyAlienTexture);
  if (IsTextureReady(enemyUfoTexture))
    UnloadTexture(enemyUfoTexture);

  if (IsTextureReady(bossForestTexture))
    UnloadTexture(bossForestTexture);
  if (IsTextureReady(bossCaveTexture))
    UnloadTexture(bossCaveTexture);
  if (IsTextureReady(bossCityTexture))
    UnloadTexture(bossCityTexture);
  if (IsTextureReady(bossSpaceTexture))
    UnloadTexture(bossSpaceTexture);

  if (IsTextureReady(bgForestTexture))
    UnloadTexture(bgForestTexture);
  if (IsTextureReady(bgCaveTexture))
    UnloadTexture(bgCaveTexture);
  if (IsTextureReady(bgCityTexture))
    UnloadTexture(bgCityTexture);
  if (IsTextureReady(bgSpaceTexture))
    UnloadTexture(bgSpaceTexture);

  if (IsSoundReady(collectStarSound))
    UnloadSound(collectStarSound);
  if (IsSoundReady(hitEnemySound))
    UnloadSound(hitEnemySound);
  if (IsSoundReady(gameOverSound))
    UnloadSound(gameOverSound);
  if (IsSoundReady(shootSound))
    UnloadSound(shootSound);
  if (IsMusicReady(bgMusic))
    UnloadMusicStream(bgMusic);

  // Giải phóng phông chữ vector
  if (gameFont.texture.id != GetFontDefault().texture.id) {
    UnloadFont(gameFont);
  }
  UnloadSkills();
  if (gs->bloomExtractShaderLoaded)
    UnloadShader(gs->bloomExtractShader);
  if (gs->bloomBlurShaderLoaded)
    UnloadShader(gs->bloomBlurShader);
  if (gs->vaporShaderLoaded)
    UnloadShader(gs->vaporShader);
}
