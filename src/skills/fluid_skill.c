#include "fluid_skill.h"
#include "config.h"
#include "enemy.h"
#include "game.h"
#include "particle.h"
#include "raymath.h"
#include "resource_loader.h"
#include <math.h>
#include <stddef.h>

#define PARTICLES_PER_SECOND 225.0f
#define SPAWN_INTERVAL (1.0f / PARTICLES_PER_SECOND)
#define MAX_FLUID_PARTICLES 3000
#define MAX_EMITTERS 4

// Cấu trúc Nguồn phát tia
typedef struct {
  bool active;
  Vector2 startPos;
  Vector2 targetPos;
  Vector2 p1;
  Vector2 p2;
  float progress;
  float durationTimer;
  float spawnAccumulator;
  float twistPhase;
} StreamEmitter;

// Cấu trúc Hạt nước độc lập
typedef struct {
  Vector2 position;
  Vector2 velocity;
  Vector2 startPos, p1, p2, targetPos; // Lưu trữ quỹ đạo cá nhân
  float progress;
  float speed;
  float radius;
  float lifetime;
  float maxLifetime;
  float spreadOffset;
  float twistPhase;
  int type; // 0: Stream, 2: Splash
  bool active;
} WaterParticle;

static WaterParticle waterPool[MAX_FLUID_PARTICLES];
static StreamEmitter emitters[MAX_EMITTERS];
static int lastUsedParticle = 0;

static RenderTexture2D canvasTexture;
static Shader fluidShader;
static int timeLoc;

static Vector2 GetBezierPoint(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3,
                              float t) {
  float u = 1.0f - t;
  float tt = t * t;
  float uu = u * u;
  float uuu = uu * u;
  float ttt = tt * t;

  Vector2 p = Vector2Scale(p0, uuu);
  p = Vector2Add(p, Vector2Scale(p1, 3.0f * uu * t));
  p = Vector2Add(p, Vector2Scale(p2, 3.0f * u * tt));
  p = Vector2Add(p, Vector2Scale(p3, ttt));
  return p;
}

static void SpawnFluidParticle(int type, Vector2 pos, Vector2 vel, float speed,
                               float progress, float maxLife, float baseRadius,
                               StreamEmitter *em) {
  for (int i = 0; i < MAX_FLUID_PARTICLES; i++) {
    int index = (lastUsedParticle + i) % MAX_FLUID_PARTICLES;
    if (!waterPool[index].active) {
      waterPool[index].type = type;
      waterPool[index].position = pos;
      waterPool[index].velocity = vel;
      waterPool[index].progress = progress;
      waterPool[index].speed = speed;
      waterPool[index].radius = baseRadius;
      waterPool[index].maxLifetime = maxLife;
      waterPool[index].lifetime = maxLife;
      waterPool[index].spreadOffset = (float)GetRandomValue(-15, 15);
      waterPool[index].active = true;

      // Nếu hạt sinh ra từ luồng, copy quỹ đạo của luồng cho nó tự bay
      if (em != NULL) {
        waterPool[index].startPos = em->startPos;
        waterPool[index].targetPos = em->targetPos;
        waterPool[index].p1 = em->p1;
        waterPool[index].p2 = em->p2;
        waterPool[index].twistPhase = em->twistPhase;
      }

      lastUsedParticle = (index + 1) % MAX_FLUID_PARTICLES;
      break;
    }
  }
}

void InitFluidSkill(void) {
  canvasTexture = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
  SetTextureFilter(canvasTexture.texture, TEXTURE_FILTER_POINT);

  if (FileExists(SHADER_PATH("fluid.fs"))) {
    fluidShader = LoadShader(NULL, SHADER_PATH("fluid.fs"));
  }
  timeLoc = GetShaderLocation(fluidShader, "u_time");

  for (int i = 0; i < MAX_FLUID_PARTICLES; i++)
    waterPool[i].active = false;
  for (int i = 0; i < MAX_EMITTERS; i++)
    emitters[i].active = false;
}

static void EmitFluidStream(Vector2 startPos, Vector2 target, float twistPhase) {
  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (!emitters[i].active) {
      emitters[i].active = true;
      emitters[i].startPos = startPos;
      emitters[i].targetPos = target;
      emitters[i].progress = 0.0f;
      emitters[i].spawnAccumulator = 0.0f;
      emitters[i].durationTimer = 0.8f;
      emitters[i].twistPhase = twistPhase;

      float midX = (startPos.x + target.x) / 2.0f;
      float curveSign = (twistPhase == 0.0f) ? -1.0f : 1.0f;

      emitters[i].p1 =
          (Vector2){midX + curveSign * 100.0f,
                    startPos.y - curveSign * (float)GetRandomValue(150, 250)};
      emitters[i].p2 =
          (Vector2){midX - curveSign * 100.0f,
                    target.y + curveSign * (float)GetRandomValue(150, 250)};
      break;
    }
  }

  // Nổ bọt nước ở tay (Muzzle Flash)
  int burstCount = GetRandomValue(15, 25);
  for (int s = 0; s < burstCount; s++) {
    Vector2 burstVel = {(float)GetRandomValue(-150, 250),
                        (float)GetRandomValue(-250, 250)};
    float burstRad = (float)GetRandomValue(8, 20);
    float burstLife = (float)GetRandomValue(3, 8) / 10.0f;
    SpawnFluidParticle(2, startPos, burstVel, 0, 0, burstLife, burstRad, NULL);
  }
}

void CastFluidSkill(struct GameState *gs) {
  if (gs == NULL) return;

  // Tìm mục tiêu (quái vật gần nhất)
  int targetIdx = -1;
  float minDist = 999999.0f;
  for (int e = 0; e < MAX_ENEMIES; e++) {
    if (gs->enemyPool.enemies[e].active) {
      if (gs->enemyPool.enemies[e].type == ENEMY_GHOST &&
          !gs->enemyPool.enemies[e].isVisible)
        continue;
      float dist = Vector2Distance(gs->witch.position,
                                   gs->enemyPool.enemies[e].position);
      if (dist < minDist) {
        minDist = dist;
        targetIdx = e;
      }
    }
  }

  Vector2 targetPos;
  if (targetIdx != -1) {
    targetPos = gs->enemyPool.enemies[targetIdx].position;
  } else {
    targetPos = (Vector2){gs->witch.position.x + 800.0f, gs->witch.position.y};
  }

  // Bắn luồng chính
  EmitFluidStream(gs->witch.position, targetPos, 0.0f);
  
  // Nếu có từ 2 sao Thủy trở lên, bắn thêm luồng phụ đối xứng xoắn kép (PI)
  if (gs->witch.effectiveThuyStars >= 2) {
    EmitFluidStream(gs->witch.position, targetPos, PI);
  }

  // Thiết lập hồi chiêu và trạng thái hoạt ảnh cho phù thủy
  int lvl = gs->witch.skillLevels[SKILL_ICE_BLAST];
  gs->witch.skillCooldown[SKILL_ICE_BLAST] = fmaxf(2.0f, 4.5f - 0.3f * (lvl - 1));
  gs->witch.animState = WITCH_STATE_ATTACK_WEAPON;
  gs->witch.stateTimer = 0.35f;
}

void UpdateFluidSkill(struct GameState *gs, float dt) {
  if (gs == NULL)
    return;
  float time = (float)GetTime();
  
  static float hitSoundCooldown = 0.0f;
  if (hitSoundCooldown > 0.0f) {
    hitSoundCooldown -= dt;
  }

  // Khởi tạo và cập nhật thời gian giãn cách trúng đòn cho từng quái vật
  static float enemyHitTimers[MAX_ENEMIES] = {0};
  for (int e = 0; e < MAX_ENEMIES; e++) {
    if (enemyHitTimers[e] > 0.0f) {
      enemyHitTimers[e] -= dt;
    }
  }

  // 1. CẬP NHẬT CÁC ĐẦU PHUN NƯỚC (EMITTERS) VÀ XỬ LÝ VA CHẠM DỌC THEO ĐƯỜNG CONG (CURVE COLLISION)
  for (int e = 0; e < MAX_EMITTERS; e++) {
    if (!emitters[e].active)
      continue;

    float commonSpeed = 1.5f;
    if (emitters[e].progress < 1.0f) {
      emitters[e].progress += dt * commonSpeed;
      if (emitters[e].progress > 1.0f)
        emitters[e].progress = 1.0f;
    }

    emitters[e].durationTimer -= dt;
    if (emitters[e].durationTimer <= 0.0f) {
      emitters[e].active = false;
      continue;
    }

    emitters[e].spawnAccumulator += dt;
    int maxSpawnsThisFrame = 30;
    while (emitters[e].spawnAccumulator >= SPAWN_INTERVAL &&
           maxSpawnsThisFrame-- > 0) {
      Vector2 zeroVel = {0, 0};
      float baseRad = (float)GetRandomValue(25, 45);
      SpawnFluidParticle(0, emitters[e].startPos, zeroVel, commonSpeed, 0.0f,
                         1.2f, baseRad, &emitters[e]);
      emitters[e].spawnAccumulator -= SPAWN_INTERVAL;
    }

    // TỐI ƯU HÓA: Quét va chạm dọc theo đường cong Bezier của luồng nước (Không quét theo hạt)
    // Chỉ lấy mẫu 6 điểm dọc theo luồng nước đang phun để tính toán va chạm (tiết kiệm CPU tối đa)
    int numSamples = 6;
    for (int s = 0; s < numSamples; s++) {
      float t_sample = emitters[e].progress * ((float)s / (float)(numSamples - 1));
      Vector2 pos = GetBezierPoint(emitters[e].startPos, emitters[e].p1, emitters[e].p2, emitters[e].targetPos, t_sample);

      // Thêm độ nhiễu sóng tương ứng với hình dạng của hạt nước
      float focusFactor = 1.0f - powf(t_sample, 4.0f);
      float waveFreq = t_sample * 15.0f - time * 25.0f;
      float noiseX = sinf(waveFreq + emitters[e].twistPhase) * (15.0f * focusFactor);
      float noiseY = cosf(waveFreq * 1.2f + emitters[e].twistPhase) * (15.0f * focusFactor);
      pos.x += noiseX;
      pos.y += noiseY;

      // Quét va chạm với tất cả quái vật
      for (int enemyIdx = 0; enemyIdx < MAX_ENEMIES; enemyIdx++) {
        if (gs->enemyPool.enemies[enemyIdx].active) {
          Enemy *enemy = &gs->enemyPool.enemies[enemyIdx];
          if (enemy->type == ENEMY_GHOST && !enemy->isVisible)
            continue;
          if (enemyHitTimers[enemyIdx] > 0.0f)
            continue; // Đang trong thời gian giãn cách nhận dame của chiêu này

          float dist = Vector2Distance(pos, enemy->position);
          float colliderRadius = 25.0f; // Bán kính vùng ảnh hưởng của tia nước
          if (dist < (colliderRadius + enemy->collisionRadius)) {
            int lvl = gs->witch.skillLevels[SKILL_ICE_BLAST];
            // Sát thương theo nhịp (tick damage) cân bằng
            int damage = (10 + 4 * gs->witch.effectiveThuyStars) * (1.0f + 0.15f * (lvl - 1));

            bool killed = DamageEnemy(&gs->enemyPool, enemyIdx, damage);
            if (killed) {
              gs->witch.score += (enemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
              gs->witch.gold += GetEnemyGoldDrop(enemy->type);
              SpawnExplosion(&gs->particlePool, enemy->position, 14, RED);
            }

            // Giãn cách phát âm thanh trúng đòn
            if (hitSoundCooldown <= 0.0f) {
              if (IsSoundReady(hitEnemySound))
                PlaySound(hitEnemySound);
              hitSoundCooldown = 0.08f;
            }

            // Sét đặt giãn cách nhận sát thương tiếp theo cho quái vật này (150ms)
            enemyHitTimers[enemyIdx] = 0.15f;

            // Nổ bọt nước thẩm mỹ tại điểm va chạm
            int splashCount = GetRandomValue(3, 5);
            for (int sp = 0; sp < splashCount; sp++) {
              Vector2 splashVel = {(float)GetRandomValue(-300, 300),
                                   (float)GetRandomValue(-400, 100)};
              float splashRad = (float)GetRandomValue(5, 12);
              float splashLife = (float)GetRandomValue(3, 6) / 10.0f;
              SpawnFluidParticle(2, pos, splashVel, 0, 0, splashLife, splashRad, NULL);
            }
          }
        }
      }
    }
  }

  // 2. CẬP NHẬT CÁC HẠT NƯỚC (HIỆU ỨNG THẨM MỸ - 100% KHÔNG TÍNH TOÁN VA CHẠM)
  for (int i = 0; i < MAX_FLUID_PARTICLES; i++) {
    if (!waterPool[i].active)
      continue;

    waterPool[i].lifetime -= dt;

    if (waterPool[i].lifetime <= 0.0f) {
      waterPool[i].active = false;
      continue;
    }

    if (waterPool[i].type == 2) { // Splash
      waterPool[i].position.x += waterPool[i].velocity.x * dt;
      waterPool[i].position.y += waterPool[i].velocity.y * dt;
      waterPool[i].velocity.y += 800.0f * dt;
      continue;
    }

    // Stream
    waterPool[i].progress += waterPool[i].speed * dt;

    if (waterPool[i].progress >= 1.0f) {
      waterPool[i].active = false;
      int splashCount = GetRandomValue(3, 6);
      for (int s = 0; s < splashCount; s++) {
        Vector2 splashVel = {(float)GetRandomValue(-250, 200),
                             (float)GetRandomValue(-400, 50)};
        float splashRad = (float)GetRandomValue(5, 20);
        float splashLife = (float)GetRandomValue(4, 10) / 10.0f;
        SpawnFluidParticle(2, waterPool[i].position, splashVel, 0, 0,
                           splashLife, splashRad, NULL);
      }
      continue;
    }

    Vector2 basePos =
        GetBezierPoint(waterPool[i].startPos, waterPool[i].p1, waterPool[i].p2,
                       waterPool[i].targetPos, waterPool[i].progress);

    float focusFactor = 1.0f - powf(waterPool[i].progress, 4.0f);
    float waveFreq = waterPool[i].progress * 15.0f - time * 25.0f;

    float noiseX =
        sinf(waveFreq + waterPool[i].spreadOffset + waterPool[i].twistPhase) *
        (15.0f * focusFactor);
    float noiseY = cosf(waveFreq * 1.2f - waterPool[i].spreadOffset +
                        waterPool[i].twistPhase) *
                    (15.0f * focusFactor);

    waterPool[i].position.x = basePos.x + noiseX;
    waterPool[i].position.y = basePos.y + noiseY;
  }
}

void DrawFluidSkill(struct GameState *gs) {
  // Kiểm tra xem có hạt nước hoặc đầu phun nào đang hoạt động không
  bool hasActive = false;
  for (int i = 0; i < MAX_FLUID_PARTICLES; i++) {
    if (waterPool[i].active) {
      hasActive = true;
      break;
    }
  }
  if (!hasActive) {
    for (int i = 0; i < MAX_EMITTERS; i++) {
      if (emitters[i].active) {
        hasActive = true;
        break;
      }
    }
  }

  // Nếu không có hạt hay đầu phun nào hoạt động, không cần vẽ gì hết
  if (!hasActive)
    return;

  float time = (float)GetTime();

  // Tạm dừng vẽ lên virtualCanvas
  EndTextureMode();

  BeginTextureMode(canvasTexture);
  ClearBackground(BLANK);
  BeginBlendMode(BLEND_ALPHA);
  for (int i = 0; i < MAX_FLUID_PARTICLES; i++) {
    if (!waterPool[i].active)
      continue;

    float currentRadius = waterPool[i].radius;
    float currentAlpha = 0.6f;
    float lifeRatio = waterPool[i].lifetime / waterPool[i].maxLifetime;

    if (waterPool[i].type < 2) {
      float bulge = sinf(waterPool[i].progress * PI);
      float sizeScale = 0.3f + (0.7f * bulge);
      float fadeScale = (lifeRatio < 0.3f) ? (lifeRatio / 0.3f) : 1.0f;
      currentRadius *= (sizeScale * fadeScale);
      currentAlpha *= fadeScale;
    } else {
      currentRadius *= lifeRatio;
      currentAlpha = lifeRatio * 1.0f;
    }

    DrawCircleGradient((int)waterPool[i].position.x,
                       (int)waterPool[i].position.y, currentRadius,
                       ColorAlpha(WHITE, currentAlpha), BLANK);
  }
  EndBlendMode();
  EndTextureMode();

  // Tiếp tục vẽ lên virtualCanvas
  BeginTextureMode(gs->virtualCanvas);

  SetShaderValue(fluidShader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
  BeginShaderMode(fluidShader);
  DrawTextureRec(canvasTexture.texture,
                 (Rectangle){0, 0, (float)canvasTexture.texture.width,
                             (float)-canvasTexture.texture.height},
                 (Vector2){0, 0}, WHITE);
  EndShaderMode();
}

void DrawFluidSkillLightmap(struct GameState *gs) {
  if (gs == NULL)
    return;

  bool hasActive = false;
  for (int i = 0; i < MAX_FLUID_PARTICLES; i++) {
    if (waterPool[i].active) {
      hasActive = true;
      break;
    }
  }
  if (!hasActive)
    return;

  BeginBlendMode(BLEND_ADDITIVE);
  for (int i = 0; i < MAX_FLUID_PARTICLES; i++) {
    if (waterPool[i].active) {
      float r = waterPool[i].radius * 2.2f;
      float lifeRatio = waterPool[i].lifetime / waterPool[i].maxLifetime;
      float glowAlpha = 0.35f * lifeRatio;

      DrawCircleGradient(waterPool[i].position.x, waterPool[i].position.y, r,
                         ColorAlpha(SKYBLUE, glowAlpha), BLANK);
      DrawCircleGradient(waterPool[i].position.x, waterPool[i].position.y,
                         r * 0.4f, ColorAlpha(WHITE, glowAlpha * 1.5f), BLANK);
    }
  }
  EndBlendMode();
}

void UnloadFluidSkill(void) {
  UnloadShader(fluidShader);
  UnloadRenderTexture(canvasTexture);
}
