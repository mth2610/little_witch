#ifndef ENEMY_BOSS_H
#define ENEMY_BOSS_H

#include "raylib.h"
#include "enemy.h"

struct GameState;

// Cập nhật đòn tấn công đặc biệt của Trùm
void UpdateBossAttack(EnemyPool *pool, Enemy *boss, struct GameState *gs, float deltaTime);

// Vẽ đồ họa vector (thủ thuật) cho Trùm
void DrawProceduralBoss(const Enemy *e, Color col);

#endif // ENEMY_BOSS_H
