// ============================================================
// gameplay.c — Cập nhật logic gameplay màn chơi chính
// ============================================================

#include "gameplay.h"
#include "biome.h"
#include "collision.h"
#include "config.h"
#include "enemy.h"
#include "game.h"
#include "particle.h"
#include "raylib.h"
#include "raymath.h"
#include "skill.h"
#include "star.h"
#include "trail.h"
#include "ui.h"
#include "witch.h"
#include <math.h>
#include <stdlib.h>

// Extern âm thanh
extern Sound gameOverSound;
extern Music bgMusic;

// Sinh lựa chọn kỹ năng sau khi diệt boss
void GenerateSkillChoices(GameState *gs) {
  // Chọn 3 kỹ năng duy nhất ngẫu nhiên từ 7 kỹ năng có sẵn (từ index 1 đến 7)
  int pool[7] = {SKILL_SHURIKEN, SKILL_POISON_CLOUD, SKILL_ICE_BLAST,
                 SKILL_FIREBALL, SKILL_LIGHTNING,    SKILL_MAGNET,
                 SKILL_SHIELD};

  // Trộn ngẫu nhiên Fisher-Yates
  for (int i = 6; i > 0; i--) {
    int j = GetRandomValue(0, i);
    int temp = pool[i];
    pool[i] = pool[j];
    pool[j] = temp;
  }

  gs->skillChoices[0] = pool[0];
  gs->skillChoices[1] = pool[1];
  gs->skillChoices[2] = pool[2];
}

// Gọi khi tiêu diệt boss để bắt đầu quá trình nhận thẻ kỹ năng và chuyển vùng
// đất tiếp theo
void AdvanceToNextBiome(GameState *gs) {
  if (gs == NULL)
    return;

  // 1. Sinh các hạt bụi ăn mừng nhiều màu sắc rực rỡ xung quanh phù thủy
  for (int i = 0; i < 45; i++) {
    float vx = (float)GetRandomValue(-250, 250);
    float vy = (float)GetRandomValue(-250, 250);
    Color colors[] = {GOLD, YELLOW, ORANGE, WHITE, PINK, SKYBLUE, LIME, PURPLE};
    Color col = colors[GetRandomValue(0, 7)];
    float lifetime = 1.2f + ((float)GetRandomValue(0, 10) / 10.0f);
    float size = 4.5f + ((float)GetRandomValue(0, 40) / 10.0f);
    SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){vx, vy},
                  lifetime, size, col);
  }

  // Rung màn hình cực mạnh ăn mừng chiến thắng
  gs->shakeIntensity = 8.0f;
  gs->shakeDuration = 0.45f;

  // 2. Sinh các lựa chọn kỹ năng ngẫu nhiên cho màn hình chọn thẻ
  GenerateSkillChoices(gs);

  // 3. Chuyển sang màn hình chọn kỹ năng (SCREEN_SKILL_SELECT) để người chơi
  // nâng cấp
  gs->currentScreen = SCREEN_SKILL_SELECT;

  // 4. Tắt cờ hoạt động của boss hiện tại
  gs->enemyPool.bossActive = false;
  gs->enemyPool.bossIndex = -1;
}

// Cập nhật trạng thái gameplay màn chơi chính
void UpdateGameplay(GameState *gs, float deltaTime) {
  if (gs == NULL)
    return;

  // 1. CẬP NHẬT PHÙ THỦY & VỆT KHÓI KHÍ ĐỘNG HỌC
  UpdateWitch(&gs->witch, deltaTime);
  float scrollSpeed = GetBiomeConfig(gs->currentBiome).scrollSpeed;
  UpdateWitchTrail(&gs->trail, &gs->witch, deltaTime, scrollSpeed);

  // Sinh hạt bụi gió lướt bay phía sau phù thủy để tăng cảm giác tốc độ và gió
  // thổi
  if (GetRandomValue(0, 100) < 35) {
    float dir = gs->witch.facingRight ? -1.0f : 1.0f;
    Vector2 pPos = {gs->witch.position.x + dir * -25.0f,
                    gs->witch.position.y + (float)GetRandomValue(-35, 35)};
    Vector2 pVel = {dir * (220.0f + (float)GetRandomValue(0, 120)),
                    (float)GetRandomValue(-15, 15)};
    float life = 0.30f + (float)GetRandomValue(0, 30) / 100.0f;
    float rad = 1.2f + (float)GetRandomValue(0, 20) / 10.0f;

    // Màu sắc của hạt gió lướt bay đồng bộ với màu vệt khói khí động học
    Color trailCol = GetWitchTrailColor(&gs->witch);
    Color pCol = ColorAlpha(trailCol, 0.40f);
    SpawnParticle(&gs->particlePool, pPos, pVel, life, rad, pCol);
  }

  // Nếu hết mạng thì Game Over ngay lập tức
  if (gs->witch.lives <= 0) {
    gs->currentScreen = SCREEN_GAMEOVER;
    gs->shakeDuration = 0.0f;
    gs->shakeIntensity = 0.0f;
    if (IsSoundReady(gameOverSound))
      PlaySound(gameOverSound);
    return;
  }

  // 2. PHÁT TIẾNG BGM NẾU CÓ
  if (IsMusicReady(bgMusic)) {
    UpdateMusicStream(bgMusic);
  }

  // 3. XỬ LÝ SPAWN SAO TỰ DO
  gs->starPool.spawnTimer += deltaTime;
  // Tốc độ spawn sao tăng gấp đôi (giảm interval một nửa) ở vùng Space
  float effectiveStarInterval = (gs->currentBiome == BIOME_SPACE) ? 1.5f : 3.0f;

  int activeStarCount = 0;
  for (int i = 0; i < MAX_STARS; i++) {
    if (gs->starPool.stars[i].active)
      activeStarCount++;
  }

  if (gs->starPool.spawnTimer >= effectiveStarInterval &&
      activeStarCount < MAX_STARS) {
    // Sinh tọa độ ngẫu nhiên bắt đầu từ ngoài màn hình bên phải để bay vào
    float sx = VIRTUAL_WIDTH + 40.0f;
    float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);

    // Tính tỷ lệ sao dựa trên Nâng cấp May mắn
    int luckLevel = gs->witch.upgrades.level[UPGRADE_LUCK];
    float rainbowChance =
        0.05f + 0.10f * luckLevel; // Cơ bản 5%, tăng 10% mỗi cấp
    float goldChance = 0.15f + 0.10f * luckLevel; // Cơ bản 15%

    float roll = (float)GetRandomValue(0, 100) / 100.0f;
    StarType type = STAR_KIM;

    if (roll < rainbowChance) {
      type = STAR_RAINBOW;
    } else {
      // Chọn ngẫu nhiên 5 hệ ngũ hành: Kim, Mộc, Thủy, Hỏa, Thổ
      type = (StarType)GetRandomValue(STAR_KIM, STAR_THO);
    }

    SpawnStar(&gs->starPool, sx, sy, type);
    gs->starPool.spawnTimer = 0.0f;
  }

  // 4. XỬ LÝ SPAWN QUÁI THƯỜNG
  // Nếu chưa xuất hiện Trùm và chưa diệt trùm của Biome này
  if (!gs->enemyPool.bossActive && !gs->bossSpawned) {
    int threshold = (gs->currentBiome == BIOME_FOREST) ? 600 : BOSS_SPAWN_SCORE;

    // Nếu điểm đạt mốc threshold tính từ đầu biome thì triệu hồi trùm
    if (gs->witch.score >= gs->biomeStartScore + threshold) {
      SpawnBoss(&gs->enemyPool, gs->currentBiome);
      gs->bossSpawned = true;
    } else {
      // Ngược lại, tiếp tục sinh quái thường
      gs->enemyPool.spawnTimer += deltaTime;
      BiomeConfig biomeCfg = GetBiomeConfig(gs->currentBiome);
      float spawnRateInterval = 1.0f / biomeCfg.enemySpawnRate;

      if (gs->currentBiome == BIOME_FOREST) {
        int relativeScore = gs->witch.score - gs->biomeStartScore;
        if (relativeScore < 150) {
          // Phase 1: Forest Silence (0 to 150 score)
          spawnRateInterval = 2.5f;
          if (gs->enemyPool.spawnTimer >= spawnRateInterval) {
            float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
            SpawnEnemy(&gs->enemyPool, ENEMY_SLIME, VIRTUAL_WIDTH + 60.0f, sy);
            gs->enemyPool.spawnTimer = 0.0f;
          }
        } else if (relativeScore < 400) {
          // Phase 2: Shadowy Canopy (150 to 400 score)
          spawnRateInterval = 1.8f;
          if (gs->enemyPool.spawnTimer >= spawnRateInterval) {
            float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
            EnemyType spawnType =
                (GetRandomValue(0, 100) < 60) ? ENEMY_SLIME : ENEMY_BAT;
            SpawnEnemy(&gs->enemyPool, spawnType, VIRTUAL_WIDTH + 60.0f, sy);

            // 20% tỉ lệ sinh thêm quái đôi ở vị trí lệch Y
            if (GetRandomValue(0, 100) < 20) {
              float sy2 = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
              if (fabsf(sy2 - sy) < 100.0f) {
                sy2 = fmodf(sy2 + 200.0f, VIRTUAL_HEIGHT - 120.0f) + 60.0f;
              }
              EnemyType spawnType2 =
                  (GetRandomValue(0, 100) < 60) ? ENEMY_SLIME : ENEMY_BAT;
              SpawnEnemy(&gs->enemyPool, spawnType2, VIRTUAL_WIDTH + 60.0f,
                         sy2);
            }
            gs->enemyPool.spawnTimer = 0.0f;
          }
        } else {
          // Phase 3: The Gathering Storm (400 to 600 score)
          spawnRateInterval = 1.2f;
          if (gs->enemyPool.spawnTimer >= spawnRateInterval) {
            float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
            EnemyType spawnType =
                (GetRandomValue(0, 100) < 40) ? ENEMY_SLIME : ENEMY_BAT;
            SpawnEnemy(&gs->enemyPool, spawnType, VIRTUAL_WIDTH + 60.0f, sy);

            // Bắt đầu rung màn hình nhẹ từ 500 điểm báo hiệu boss chuẩn bị trỗi
            // dậy
            if (relativeScore >= 500 && fmodf(gs->frameCount, 180.0f) < 8.0f) {
              gs->shakeIntensity = 2.0f;
              gs->shakeDuration = 0.12f;
            }
            gs->enemyPool.spawnTimer = 0.0f;
          }
        }
      } else {
        // Các biome khác giữ nguyên cơ chế spawn quái thường mặc định
        if (gs->enemyPool.spawnTimer >= spawnRateInterval) {
          int typeIdx = GetRandomValue(0, biomeCfg.enemyTypeCount - 1);
          EnemyType spawnType = biomeCfg.enemyTypes[typeIdx];

          float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
          SpawnEnemy(&gs->enemyPool, spawnType, VIRTUAL_WIDTH + 60.0f, sy);

          gs->enemyPool.spawnTimer = 0.0f;
        }
      }
    }
  }

  // 5. KÍCH HOẠT KỸ NĂNG CHỦ ĐỘNG (RPG ACTIVE SKILLS & BUFFS)
  for (int s = 1; s < SKILL_COUNT; s++) {
    if (!gs->witch.activeSkills[s])
      continue;

    bool keyPressed = false;
    if (s == SKILL_SHURIKEN && IsKeyPressed(KEY_ONE))
      keyPressed = true;
    else if (s == SKILL_POISON_CLOUD && IsKeyPressed(KEY_TWO))
      keyPressed = true;
    else if (s == SKILL_ICE_BLAST && IsKeyPressed(KEY_THREE))
      keyPressed = true;
    else if (s == SKILL_FIREBALL && IsKeyPressed(KEY_FOUR))
      keyPressed = true;
    else if (s == SKILL_LIGHTNING && IsKeyPressed(KEY_FIVE))
      keyPressed = true;
    else if (s == SKILL_MAGNET && IsKeyPressed(KEY_E))
      keyPressed = true;
    else if (s == SKILL_SHIELD && IsKeyPressed(KEY_Q))
      keyPressed = true;

    Vector2 center = GetSkillButtonCenter(s);
    bool buttonPressed = IsCircleButtonClicked(center, 28.0f);

    if ((keyPressed || buttonPressed) && gs->witch.skillCooldown[s] <= 0.0f) {
      CastActiveSkill(gs, s);
    }
  }

  // 5.5 TẤN CÔNG ĐÁNH THƯỜNG (PC: SPACEBAR, MOBILE: NÚT BẤM TRÒN LỚN)
  Vector2 attackCenter = {1150.0f, 600.0f};
  bool normalAttackBtnPressed = IsCircleButtonClicked(attackCenter, 48.0f);

  if (IsKeyPressed(KEY_SPACE) || IsKeyDown(KEY_SPACE) ||
      normalAttackBtnPressed) {
    if (gs->witch.keyboardAttackCooldown <= 0.0f) {
      // Gọi hàm tấn công thường đã được module hóa
      CastNormalAttack(gs);

      // Thiết lập hồi chiêu đánh thường bằng phím cách (0.35s)
      gs->witch.keyboardAttackCooldown = 0.35f;
    }
  }

  // 6. CẬP NHẬT CÁC ĐỐI TƯỢNG TRONG POOL
  bool hasMagnet = true; // Nam châm bị động hệ Kim luôn kích hoạt
  BiomeConfig biomeCfg = GetBiomeConfig(gs->currentBiome);
  UpdateStars(&gs->starPool, &gs->witch, deltaTime, hasMagnet,
              biomeCfg.scrollSpeed, &gs->particlePool);
  UpdateEnemies(gs, deltaTime);

  // Cập nhật lan truyền sát thương sét chuỗi Chain Lightning thời gian thực
  UpdateSkills(gs, deltaTime);
  UpdateProjectiles(&gs->projectilePool, deltaTime, &gs->particlePool,
                    &gs->enemyPool);
  UpdateParticles(&gs->particlePool, deltaTime);

  // Sinh hạt bụi sáng trôi nổi trong không khí (Ambient floating dust) phong
  // cách Ori
  if (GetRandomValue(0, 100) < 12) {
    Vector2 pos = {VIRTUAL_WIDTH + 15.0f,
                   (float)GetRandomValue(10, VIRTUAL_HEIGHT - 10)};
    Vector2 vel = {-(45.0f + (float)GetRandomValue(0, 50)),
                   (float)GetRandomValue(-8, 8)};
    float lifetime = 5.0f + ((float)rand() / RAND_MAX) * 3.0f;
    float size = 1.2f + ((float)rand() / RAND_MAX) * 1.8f;

    Color col = LIME;
    if (gs->currentBiome == BIOME_FOREST)
      col = (GetRandomValue(0, 1) == 0 ? LIME : GOLD);
    else if (gs->currentBiome == BIOME_CAVE)
      col = (GetRandomValue(0, 1) == 0 ? SKYBLUE : VIOLET);
    else if (gs->currentBiome == BIOME_CITY)
      col = (GetRandomValue(0, 1) == 0 ? MAGENTA : CYAN);
    else if (gs->currentBiome == BIOME_SPACE)
      col = (GetRandomValue(0, 1) == 0 ? PINK : SKYBLUE);

    SpawnParticle(&gs->particlePool, pos, vel, lifetime, size, col);
  }

  // 7. XỬ LÝ COOLDOWN RUNG MÀN HÌNH (SCREEN SHAKE)
  if (gs->shakeDuration > 0.0f) {
    gs->shakeDuration -= deltaTime;
    gs->shakeIntensity = Lerp(gs->shakeIntensity, 0.0f, deltaTime * 6.0f);
    if (gs->shakeDuration <= 0.0f) {
      gs->shakeDuration = 0.0f;
      gs->shakeIntensity = 0.0f;
    }
  }

  // Cập nhật timer flash chớp sáng chiêu cuối Ultimate
  if (gs->ultimateFlashTimer > 0.0f) {
    gs->ultimateFlashTimer -= deltaTime;
    if (gs->ultimateFlashTimer < 0.0f)
      gs->ultimateFlashTimer = 0.0f;
  }

  // 8. KIỂM TRA VA CHẠM
  CheckCollisions(gs);

  // 9. KÍCH HOẠT CHIÊU CUỐI ULTIMATE: ELEMENTAL BURST BẰNG BÀN PHÍM/TOUCH
  if (gs->currentScreen == SCREEN_GAMEPLAY) {
    int rainbowCount = 0;
    int kim = 0, moc = 0, thuy = 0, hoa = 0, tho = 0;
    for (int i = 0; i < MAX_STARS; i++) {
      if (gs->starPool.stars[i].active && gs->starPool.stars[i].isOrbiting) {
        if (gs->starPool.stars[i].type == STAR_RAINBOW)
          rainbowCount++;
        else if (gs->starPool.stars[i].type == STAR_KIM)
          kim++;
        else if (gs->starPool.stars[i].type == STAR_MOC)
          moc++;
        else if (gs->starPool.stars[i].type == STAR_THUY)
          thuy++;
        else if (gs->starPool.stars[i].type == STAR_HOA)
          hoa++;
        else if (gs->starPool.stars[i].type == STAR_THO)
          tho++;
      }
    }
    bool isUltReady = (rainbowCount > 0) ||
                      (kim > 0 && moc > 0 && thuy > 0 && hoa > 0 && tho > 0);

    Vector2 ultCenter = {1150.0f, 360.0f};
    bool ultButtonClicked = IsCircleButtonClicked(ultCenter, 36.0f);

    if (isUltReady && (IsKeyPressed(KEY_F) || ultButtonClicked)) {
      TriggerUltimate(gs);
    }
  }

  // Cập nhật biến frame toàn cục để phục vụ nhấp nháy sao, v.v.
  gs->frameCount++;
}
