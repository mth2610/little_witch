#ifndef ENEMY_NORMAL_H
#define ENEMY_NORMAL_H

#include "raylib.h"
#include "enemy.h"

struct Witch;

// Cập nhật di chuyển và tấn công cho quái vật thông thường
void UpdateNormalEnemy(Enemy *e, struct Witch *witch, ProjectilePool *projPool, float deltaTime);

// Vẽ đồ họa vector (thủ thuật) cho quái vật thông thường
void DrawProceduralNormalEnemy(const Enemy *e, Color col);

#endif // ENEMY_NORMAL_H
