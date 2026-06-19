#include "shield_skill.h"
#include "game.h"
#include "resource_loader.h"
#include "config.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static Shader shieldShader;
static bool shieldShaderLoaded = false;

void InitShieldSkill(void) {
    if (FileExists(SHADER_PATH("shield.fs"))) {
        shieldShader = LoadShader(NULL, SHADER_PATH("shield.fs"));
        shieldShaderLoaded = true;
    } else {
        shieldShaderLoaded = false;
    }
}

void CastShieldSkill(struct GameState *gs) {
    int lvl = gs->witch.skillLevels[SKILL_SHIELD];
    // Buff Khiên ma pháp chủ động: kích hoạt khiên ma lực, thời gian tăng theo level, giảm hồi chiêu
    gs->witch.manaShieldHealth =
        1.67f +
        0.33f * (lvl -
                 1); // Tăng thời gian (0.33f = 1 giây khi chia 3.0f ở witch.c)
    gs->witch.skillCooldown[SKILL_SHIELD] =
        fmaxf(8.0f, 15.0f - 1.0f * (lvl - 1));

    // Chuyển tư thế DEFENSE
    gs->witch.animState = WITCH_STATE_DEFENSE;
    gs->witch.stateTimer = 0.5f;
    if (IsSoundReady(shootSound))
      PlaySound(shootSound);

    // Hiệu ứng hạt tỏa tròn từ phù thủy
    for (int i = 0; i < 15; i++) {
      float angle = ((float)i / 15.0f) * 2.0f * PI;
      float speed = 160.0f;
      Vector2 vel = {cosf(angle) * speed, sinf(angle) * speed};
      SpawnParticle(&gs->particlePool, gs->witch.position, vel, 0.4f, 2.5f,
                    SKYBLUE);
    }
}

void DrawShieldEffect(Vector2 pos, float radiusVal) {
    float timeVal = (float)GetTime();
    if (shieldShaderLoaded) {
        BeginShaderMode(shieldShader);
        int timeLoc = GetShaderLocation(shieldShader, "time");
        int glowColorLoc = GetShaderLocation(shieldShader, "glowColor");
        int centerLoc = GetShaderLocation(shieldShader, "center");
        int radiusLoc = GetShaderLocation(shieldShader, "radius");
        
        Vector4 glowCol = { 0.3f, 0.75f, 1.0f, 1.0f }; // Sky blue
        
        SetShaderValue(shieldShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shieldShader, glowColorLoc, &glowCol, SHADER_UNIFORM_VEC4);
        SetShaderValue(shieldShader, centerLoc, &pos, SHADER_UNIFORM_VEC2);
        SetShaderValue(shieldShader, radiusLoc, &radiusVal, SHADER_UNIFORM_FLOAT);
        
        int screenTexLoc = GetShaderLocation(shieldShader, "screenTexture");
        SetShaderValueTexture(shieldShader, screenTexLoc, shieldBackgroundTexture);
        
        DrawRectangleRec((Rectangle){ pos.x - radiusVal, pos.y - radiusVal, radiusVal * 2.0f, radiusVal * 2.0f }, WHITE);
        EndShaderMode();
    } else {
        // Vẽ khiên chắn vector xoay chuyển động (fallback) rộng rãi và đẹp mắt
        float rot = timeVal * 120.0f;
        DrawCircleGradient(pos.x, pos.y, radiusVal * 1.1f, ColorAlpha(SKYBLUE, 0.12f), ColorAlpha(SKYBLUE, 0.0f));
        DrawCircleLines(pos.x, pos.y, radiusVal, ColorAlpha(SKYBLUE, 0.7f));
        DrawCircleLines(pos.x, pos.y, radiusVal - 1.5f, ColorAlpha(SKYBLUE, 0.3f));
        
        // Vẽ 3 quả cầu hộ mệnh xoay tròn quanh khiên
        for (int j = 0; j < 3; j++) {
            float a = rot * DEG2RAD + j * (2.0f * PI / 3.0f);
            float sx = pos.x + cosf(a) * radiusVal;
            float sy = pos.y + sinf(a) * radiusVal;
            DrawCircleGradient(sx, sy, 14.0f, ColorAlpha(WHITE, 0.6f), ColorAlpha(SKYBLUE, 0.0f));
            DrawCircle(sx, sy, 3.5f, WHITE);
        }
    }
}

void UnloadShieldSkill(void) {
    if (shieldShaderLoaded) {
        UnloadShader(shieldShader);
        shieldShaderLoaded = false;
    }
}
