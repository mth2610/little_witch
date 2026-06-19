#include "magnet_skill.h"
#include "game.h"
#include "resource_loader.h"
#include "config.h"
#include "raymath.h"
#include <math.h>

void InitMagnetSkill(void) {
    // Không có shader hay tài nguyên đặc biệt cần tải trước cho Magnet
}

void CastMagnetSkill(struct GameState *gs) {
    int lvl = gs->witch.skillLevels[SKILL_MAGNET];
    // Buff Nam châm: kích hoạt lâu hơn theo level, giảm hồi chiêu
    gs->witch.skillActiveTimer[SKILL_MAGNET] = 6.0f + 1.5f * (lvl - 1);
    gs->witch.skillCooldown[SKILL_MAGNET] =
        fmaxf(6.0f, 12.0f - 0.8f * (lvl - 1));
    if (IsSoundReady(collectStarSound))
      PlaySound(collectStarSound);

    // Hiệu ứng hạt hội tụ vào phù thủy
    for (int i = 0; i < 20; i++) {
      float angle = ((float)i / 20.0f) * 2.0f * PI;
      float dist = 120.0f;
      Vector2 startPos = {gs->witch.position.x + cosf(angle) * dist,
                          gs->witch.position.y + sinf(angle) * dist};
      Vector2 vel = {-cosf(angle) * 250.0f, -sinf(angle) * 250.0f};
      SpawnParticle(&gs->particlePool, startPos, vel, 0.45f, 2.0f, GOLD);
    }
}

void UnloadMagnetSkill(void) {
    // Dọn dẹp nếu có
}
