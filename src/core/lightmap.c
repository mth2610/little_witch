// ============================================================
// lightmap.c — Lightmap 2D, Bloom Pipeline, và Letterbox Scaling
// ============================================================

#include "lightmap.h"
#include "config.h"
#include "enemy.h"
#include "game.h"
#include "particle.h"
#include "raylib.h"
#include "rlgl.h"
#include "skill.h"
#include "star.h"
#include "witch.h"
#include <math.h>

// Biến tỷ lệ và căn lề cho màn hình ảo (Letterbox) — dùng bởi ScreenToVirtual
static float s_scale = 1.0f;
static int s_offsetX = 0;
static int s_offsetY = 0;

// Chuyển đổi tọa độ màn hình thật sang màn hình ảo 1280x720
Vector2 ScreenToVirtual(Vector2 screenPos) {
  Vector2 virtualPos = {0.0f, 0.0f};
  virtualPos.x = (screenPos.x - s_offsetX) / s_scale;
  virtualPos.y = (screenPos.y - s_offsetY) / s_scale;
  return virtualPos;
}

// ----------------------------------------------------------------
// Vẽ bản đồ ánh sáng 2D (Lightmap) phong cách Ori
// ----------------------------------------------------------------
void RenderLightmap(GameState *gs) {
  BeginTextureMode(gs->lightmap);
  // Chọn màu tối của Biome hiện tại (Ambient Darkness)
  Color ambientColor = (Color){45, 52, 65, 255}; // Forest ambient
  if (gs->currentBiome == BIOME_FOREST) {
    int relativeScore = gs->witch.score - gs->biomeStartScore;
    if (relativeScore >= 500 && !gs->enemyPool.bossActive &&
        !gs->bossSpawned) {
      ambientColor = (Color){22, 26, 32, 255}; // Tối dần khi boss sắp đến
    } else if (gs->enemyPool.bossActive) {
      ambientColor = (Color){12, 14, 18, 255}; // Tối hẳn khi đấu boss
    }
  } else if (gs->currentBiome == BIOME_CAVE)
    ambientColor = (Color){25, 20, 32, 255}; // Cave ambient
  else if (gs->currentBiome == BIOME_CITY)
    ambientColor = (Color){45, 45, 60, 255}; // City ambient
  else if (gs->currentBiome == BIOME_SPACE)
    ambientColor = (Color){18, 12, 30, 255}; // Space ambient

  ClearBackground(ambientColor);

  BeginBlendMode(BLEND_ADDITIVE);
  // A. Ánh sáng phát ra từ Phù thủy (Màu xanh cyan/trắng dịu phong cách Ori)
  float witchPulse = sinf(gs->frameCount * 0.05f) * 12.0f;
  // Vầng sáng rộng ngoài
  DrawCircleGradient(gs->witch.position.x, gs->witch.position.y,
                     180.0f + witchPulse, ColorAlpha(SKYBLUE, 0.55f),
                     ColorAlpha(SKYBLUE, 0.0f));
  // Vầng sáng trung
  DrawCircleGradient(gs->witch.position.x, gs->witch.position.y,
                     95.0f + witchPulse * 0.5f, ColorAlpha(SKYBLUE, 0.75f),
                     ColorAlpha(SKYBLUE, 0.0f));
  // Lõi sáng trắng cực mạnh tỏa rộng bao phủ hoàn toàn thân nhân vật
  DrawCircle(gs->witch.position.x, gs->witch.position.y, 52.0f,
             ColorAlpha(WHITE, 1.0f));
  DrawCircleGradient(gs->witch.position.x, gs->witch.position.y, 80.0f,
                     ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));

  // Nếu khiên ma pháp đang hoạt động, tỏa ra vầng sáng bảo hộ khổng lồ
  if (gs->witch.manaShieldHealth > 0.0f) {
    float shieldPulse = sinf(gs->frameCount * 0.08f) * 15.0f;
    DrawCircleGradient(gs->witch.position.x, gs->witch.position.y,
                       260.0f + shieldPulse, ColorAlpha(SKYBLUE, 0.65f),
                       ColorAlpha(SKYBLUE, 0.0f));
    DrawCircleGradient(gs->witch.position.x, gs->witch.position.y, 130.0f,
                       ColorAlpha(WHITE, 0.45f), ColorAlpha(WHITE, 0.0f));
  }

  // B. Ánh sáng phát ra từ các ngôi sao bảo vệ/sao tự do
  for (int i = 0; i < MAX_STARS; i++) {
    if (gs->starPool.stars[i].active) {
      Vector2 starPos = gs->starPool.stars[i].position;
      Color starGlow = gs->starPool.stars[i].tintColor;
      float starPulse = sinf(gs->frameCount * 0.1f + i) * 3.0f;
      // Vầng sáng màu rộng hơn, rực rỡ hơn
      DrawCircleGradient(starPos.x, starPos.y, 75.0f + starPulse,
                         ColorAlpha(starGlow, 0.8f),
                         ColorAlpha(starGlow, 0.0f));
      // Lõi sáng trắng cực mạnh ở tâm
      DrawCircleGradient(starPos.x, starPos.y, 35.0f,
                         ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
      DrawCircleV(starPos, 12.0f, ColorAlpha(WHITE, 1.0f));
    }
  }

  // C. Ánh sáng phát ra từ đạn phép thuật (Projectiles)
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (gs->projectilePool.projectiles[i].active) {
      const Projectile *p = &gs->projectilePool.projectiles[i];
      if (p->isEnemy) {
        // Đạn của quái giữ nguyên vẽ thông thường
        DrawCircleGradient(p->position.x, p->position.y, 55.0f,
                           ColorAlpha(p->color, 0.75f),
                           ColorAlpha(p->color, 0.0f));
        DrawCircleGradient(p->position.x, p->position.y, 18.0f,
                           ColorAlpha(WHITE, 1.0f),
                           ColorAlpha(WHITE, 0.0f));
      } else {
        // Đạn của người chơi: Tỏa sáng theo hệ Ngũ hành
        if (p->type == PROJ_FIREBALL) {
          float flicker = 1.0f + sinf(GetTime() * 25.0f + i) * 0.15f;
          float size = 80.0f * flicker;
          DrawCircleGradient(p->position.x, p->position.y, size,
                             ColorAlpha(ORANGE, 0.75f),
                             ColorAlpha(ORANGE, 0.0f));
          DrawCircleGradient(p->position.x, p->position.y, size * 0.4f,
                             ColorAlpha(YELLOW, 0.9f),
                             ColorAlpha(YELLOW, 0.0f));
          DrawCircleGradient(p->position.x, p->position.y, size * 0.15f,
                             ColorAlpha(WHITE, 1.0f),
                             ColorAlpha(WHITE, 0.0f));
        } else if (p->type == PROJ_ICE) {
          float size = 60.0f;
          DrawCircleGradient(p->position.x, p->position.y, size * 1.5f,
                             ColorAlpha(WHITE, 0.25f),
                             ColorAlpha(WHITE, 0.0f));
          DrawCircleGradient(p->position.x, p->position.y, size,
                             ColorAlpha(CYAN, 0.8f),
                             ColorAlpha(CYAN, 0.0f));
          DrawCircleGradient(p->position.x, p->position.y, size * 0.3f,
                             ColorAlpha(WHITE, 1.0f),
                             ColorAlpha(WHITE, 0.0f));
        } else if (p->type == PROJ_POISON) {
          float pulse = 1.0f + sinf(GetTime() * 4.0f + i) * 0.12f;
          float size = 100.0f * pulse;
          DrawCircleGradient(p->position.x, p->position.y, size,
                             ColorAlpha(LIME, 0.65f),
                             ColorAlpha(LIME, 0.0f));
          DrawCircleGradient(p->position.x, p->position.y, size * 0.3f,
                             ColorAlpha(GREEN, 0.8f),
                             ColorAlpha(GREEN, 0.0f));
        } else if (p->type == PROJ_SHURIKEN) {
          float flicker = 1.0f + sinf(GetTime() * 35.0f + i) * 0.18f;
          float size = 40.0f * flicker;
          DrawCircleGradient(p->position.x, p->position.y, size,
                             ColorAlpha(GOLD, 0.9f),
                             ColorAlpha(GOLD, 0.0f));
          DrawCircleGradient(p->position.x, p->position.y, size * 0.4f,
                             ColorAlpha(WHITE, 1.0f),
                             ColorAlpha(WHITE, 0.0f));
        } else if (p->type == PROJ_TORNADO) {
          float heightVal = p->radius * 3.5f;
          float flash = (sinf(GetTime() * 50.0f + i) > 0.85f) ? 1.0f : 0.0f;
          for (int step = 0; step < 5; step++) {
            float t = step / 4.0f;
            float offsety = -t * heightVal;
            float size = p->radius * (1.5f - t * 0.8f) * 2.0f;
            Color glowColor = ColorAlpha(ORANGE, 0.4f - t * 0.2f);
            DrawCircleGradient(p->position.x, p->position.y + offsety, size,
                               glowColor, ColorAlpha(ORANGE, 0.0f));
          }
        } else {
          // Default fallback
          DrawCircleGradient(p->position.x, p->position.y, 55.0f,
                             ColorAlpha(p->color, 0.75f),
                             ColorAlpha(p->color, 0.0f));
          DrawCircleGradient(p->position.x, p->position.y, 18.0f,
                             ColorAlpha(WHITE, 1.0f),
                             ColorAlpha(WHITE, 0.0f));
        }
      }
    }
  }

  // D. Ánh sáng phát ra từ hạt bụi (Particles)
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (gs->particlePool.particles[i].active) {
      Vector2 pPos = gs->particlePool.particles[i].position;
      Color pCol = gs->particlePool.particles[i].color;
      float pAlpha = gs->particlePool.particles[i].alpha;
      float r = gs->particlePool.particles[i].radius;
      DrawCircleGradient(pPos.x, pPos.y, r * 5.0f,
                         ColorAlpha(pCol, pAlpha * 0.5f),
                         ColorAlpha(pCol, 0.0f));
      DrawCircleGradient(pPos.x, pPos.y, r * 1.8f,
                         ColorAlpha(WHITE, pAlpha * 0.7f),
                         ColorAlpha(WHITE, 0.0f));
    }
  }

  // E. Ánh sáng phát ra từ quái vật và Boss
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (gs->enemyPool.enemies[i].active) {
      Enemy *enemy = &gs->enemyPool.enemies[i];
      float pulse = sinf(gs->frameCount * 0.08f + i) * 4.0f;
      float r = enemy->collisionRadius;

      Color glowColor = RED;
      if (enemy->type == ENEMY_SLIME)
        glowColor = LIME;
      else if (enemy->type == ENEMY_BAT)
        glowColor = ORANGE;
      else if (enemy->type == ENEMY_GHOST)
        glowColor = VIOLET;
      else if (enemy->type == ENEMY_ROBOT)
        glowColor = SKYBLUE;
      else if (enemy->type == ENEMY_DRONE)
        glowColor = CYAN;
      else if (enemy->type == ENEMY_ALIEN)
        glowColor = GREEN;
      else if (enemy->type == ENEMY_UFO)
        glowColor = MAGENTA;
      else if (enemy->type >= ENEMY_BOSS_FOREST)
        glowColor = RED;

      // Bỏ qua ghost tàng hình
      if (enemy->type == ENEMY_GHOST && !enemy->isVisible)
        continue;

      // Vẽ vầng sáng màu rộng
      DrawCircleGradient(enemy->position.x, enemy->position.y,
                         r * 3.2f + pulse, ColorAlpha(glowColor, 0.6f),
                         ColorAlpha(glowColor, 0.0f));
      // Vẽ lõi sáng trắng
      DrawCircleGradient(enemy->position.x, enemy->position.y, r * 1.3f,
                         ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
      DrawCircleV(enemy->position, r * 0.7f, ColorAlpha(WHITE, 1.0f));
    }
  }

  // F. Ánh sáng phát ra từ tia sét (Chain Lightning)
  DrawSkillsLightmap(gs);

  // G. Ánh sáng phát ra từ môi trường (Đom đóm, Tinh thể, Tinh vân)
  if (gs->currentBiome == BIOME_FOREST) {
    float rawScroll = gs->bgScrollOffset;
    for (int f = 0; f < 12; f++) {
      float fireflyX = fmodf(f * 113.0f - rawScroll * 0.3f, VIRTUAL_WIDTH);
      if (fireflyX < 0)
        fireflyX += VIRTUAL_WIDTH;
      float fireflyY =
          fmodf(f * 89.0f + sinf(gs->frameCount * 0.04f + f) * 20.0f,
                VIRTUAL_HEIGHT);
      float pulse = 1.0f + sinf(gs->frameCount * 0.1f + f) * 0.4f;
      DrawCircleGradient(fireflyX, fireflyY, 50.0f * pulse,
                         ColorAlpha(LIME, 0.45f), ColorAlpha(LIME, 0.0f));
      DrawCircleGradient(fireflyX, fireflyY, 20.0f * pulse,
                         ColorAlpha(YELLOW, 0.7f),
                         ColorAlpha(YELLOW, 0.0f));
    }
  } else if (gs->currentBiome == BIOME_CAVE) {
    float rawScroll = gs->bgScrollOffset;
    for (int i = 0; i < 6; i++) {
      float cx =
          fmodf(i * 240.0f - rawScroll, VIRTUAL_WIDTH + 100.0f) - 50.0f;
      if (cx < -100.0f)
        cx += VIRTUAL_WIDTH + 100.0f;
      float pulse = 0.5f + 0.5f * sinf(gs->frameCount * 0.06f + i);
      DrawCircleGradient(cx + 40.0f, VIRTUAL_HEIGHT - 50.0f, 90.0f * pulse,
                         ColorAlpha(SKYBLUE, 0.5f),
                         ColorAlpha(SKYBLUE, 0.0f));
      DrawCircleGradient(cx + 40.0f, VIRTUAL_HEIGHT - 50.0f, 30.0f * pulse,
                         ColorAlpha(WHITE, 0.7f), ColorAlpha(WHITE, 0.0f));
    }
  } else if (gs->currentBiome == BIOME_SPACE) {
    float nebSpeed = 0.08f;
    float rawScroll = gs->bgScrollOffset;
    for (int n = 0; n < 3; n++) {
      float x =
          fmodf(n * 500.0f - rawScroll * nebSpeed, VIRTUAL_WIDTH + 400.0f) -
          200.0f;
      Vector2 center = {x, VIRTUAL_HEIGHT * 0.4f + n * 70.0f};
      Color nebColor = (n == 0) ? PURPLE : (n == 1 ? DARKBLUE : MAGENTA);
      float pulse = 1.0f + sinf(gs->frameCount * 0.02f + n) * 0.15f;
      DrawCircleGradient(center.x, center.y, 260.0f * pulse,
                         ColorAlpha(nebColor, 0.22f),
                         ColorAlpha(nebColor, 0.0f));
    }
  }

  EndBlendMode();
  EndTextureMode();
}

// ----------------------------------------------------------------
// Áp dụng lightmap lên virtual canvas (Multiply Blend)
// ----------------------------------------------------------------
void ApplyLightmap(GameState *gs) {
  BeginTextureMode(gs->virtualCanvas);
  BeginBlendMode(BLEND_MULTIPLIED);
  DrawTexturePro(
      gs->lightmap.texture,
      (Rectangle){0.0f, 0.0f, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT},
      (Rectangle){0.0f, 0.0f, (float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT},
      (Vector2){0, 0}, 0.0f, WHITE);
  EndBlendMode();
  EndTextureMode(); // Tạm dừng vẽ lên virtualCanvas để áp dụng Bloom
}

// ----------------------------------------------------------------
// Áp dụng Bloom Pipeline (extract → blur H → blur V → additive composite)
// ----------------------------------------------------------------
void ApplyBloomPipeline(GameState *gs) {
  if (gs->bloomExtractShaderLoaded && gs->bloomBlurShaderLoaded) {
    Vector2 renderSz = {320.0f, 180.0f};
    // 1. Trích xuất các pixel sáng sang bloomCanvas (320x180)
    BeginTextureMode(gs->bloomCanvas);
    ClearBackground(BLACK);
    BeginShaderMode(gs->bloomExtractShader);
    float thresholdVal = 0.75f;
    SetShaderValue(gs->bloomExtractShader,
                   GetShaderLocation(gs->bloomExtractShader, "threshold"),
                   &thresholdVal, SHADER_UNIFORM_FLOAT);
    DrawTexturePro(gs->virtualCanvas.texture,
                   (Rectangle){0.0f, 0.0f, (float)VIRTUAL_WIDTH,
                               -(float)VIRTUAL_HEIGHT},
                   (Rectangle){0.0f, 0.0f, 320.0f, 180.0f}, (Vector2){0, 0},
                   0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    // 2. Blur ngang: bloomCanvas -> blurCanvas
    BeginTextureMode(gs->blurCanvas);
    ClearBackground(BLACK);
    BeginShaderMode(gs->bloomBlurShader);
    int horiz = 1;
    SetShaderValue(gs->bloomBlurShader,
                   GetShaderLocation(gs->bloomBlurShader, "horizontal"),
                   &horiz, SHADER_UNIFORM_INT);
    SetShaderValue(gs->bloomBlurShader,
                   GetShaderLocation(gs->bloomBlurShader, "renderSize"),
                   &renderSz, SHADER_UNIFORM_VEC2);
    DrawTexturePro(gs->bloomCanvas.texture,
                   (Rectangle){0.0f, 0.0f, 320.0f, -180.0f},
                   (Rectangle){0.0f, 0.0f, 320.0f, 180.0f}, (Vector2){0, 0},
                   0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    // 3. Blur dọc: blurCanvas -> bloomCanvas
    BeginTextureMode(gs->bloomCanvas);
    ClearBackground(BLACK);
    BeginShaderMode(gs->bloomBlurShader);
    int vert = 0;
    SetShaderValue(gs->bloomBlurShader,
                   GetShaderLocation(gs->bloomBlurShader, "horizontal"),
                   &vert, SHADER_UNIFORM_INT);
    SetShaderValue(gs->bloomBlurShader,
                   GetShaderLocation(gs->bloomBlurShader, "renderSize"),
                   &renderSz, SHADER_UNIFORM_VEC2);
    DrawTexturePro(gs->blurCanvas.texture,
                   (Rectangle){0.0f, 0.0f, 320.0f, -180.0f},
                   (Rectangle){0.0f, 0.0f, 320.0f, 180.0f}, (Vector2){0, 0},
                   0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    // 4. Cộng chập Bloom (BLEND_ADDITIVE) quay lại virtualCanvas
    BeginTextureMode(gs->virtualCanvas);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTexturePro(gs->bloomCanvas.texture,
                   (Rectangle){0.0f, 0.0f, 320.0f, -180.0f},
                   (Rectangle){0.0f, 0.0f, (float)VIRTUAL_WIDTH,
                               (float)VIRTUAL_HEIGHT},
                   (Vector2){0, 0}, 0.0f, WHITE);
    EndBlendMode();
  } else {
    BeginTextureMode(gs->virtualCanvas);
  }
}

// ----------------------------------------------------------------
// Scale và vẽ virtual canvas ra màn hình thật (Letterboxing + Screen Shake)
// ----------------------------------------------------------------
void DrawVirtualCanvasToScreen(GameState *gs) {
  BeginDrawing();
  ClearBackground(BLACK);

  // Tính toán tỷ lệ co giãn tối ưu giữ nguyên aspect ratio 16:9
  float scaleX = (float)GetScreenWidth() / VIRTUAL_WIDTH;
  float scaleY = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
  s_scale = (scaleX < scaleY) ? scaleX : scaleY; // Lấy min scale

  // Tính toán khoảng đệm đen (Letterbox / Pillarbox)
  s_offsetX = (GetScreenWidth() - (int)(VIRTUAL_WIDTH * s_scale)) / 2;
  s_offsetY = (GetScreenHeight() - (int)(VIRTUAL_HEIGHT * s_scale)) / 2;

  // Cấu hình tọa độ vẽ virtual canvas
  Rectangle sourceRec = {
      0.0f, 0.0f, (float)VIRTUAL_WIDTH,
      -(float)
          VIRTUAL_HEIGHT}; // Lật ngược Y vì RenderTexture lưu tọa độ ngược
  Rectangle destRec = {(float)s_offsetX, (float)s_offsetY,
                       VIRTUAL_WIDTH * s_scale, VIRTUAL_HEIGHT * s_scale};

  // Áp dụng rung giật màn hình (Screen Shake)
  if (gs->shakeDuration > 0.0f && gs->currentScreen == SCREEN_GAMEPLAY) {
    destRec.x +=
        GetRandomValue(-gs->shakeIntensity, gs->shakeIntensity) * s_scale;
    destRec.y +=
        GetRandomValue(-gs->shakeIntensity, gs->shakeIntensity) * s_scale;
  }

  // Vẽ Canvas ra màn hình thực tế
  rlDisableColorBlend();
  DrawTexturePro(gs->virtualCanvas.texture, sourceRec, destRec,
                 (Vector2){0, 0}, 0.0f, WHITE);
  rlEnableColorBlend();

  // Vẽ lại 2 dải đen che phủ rìa ngoài để letterboxing sắc nét hoàn hảo
  if (s_offsetX > 0) {
    DrawRectangle(0, 0, s_offsetX, GetScreenHeight(), BLACK);
    DrawRectangle(GetScreenWidth() - s_offsetX, 0, s_offsetX,
                  GetScreenHeight(), BLACK);
  }
  if (s_offsetY > 0) {
    DrawRectangle(0, 0, GetScreenWidth(), s_offsetY, BLACK);
    DrawRectangle(0, GetScreenHeight() - s_offsetY, GetScreenWidth(),
                  s_offsetY, BLACK);
  }
  EndDrawing();
}
