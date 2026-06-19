#ifndef MAGNET_SKILL_H
#define MAGNET_SKILL_H

#include "raylib.h"
#include "skill.h"

struct GameState;

void InitMagnetSkill(void);
void CastMagnetSkill(struct GameState *gs);
void UnloadMagnetSkill(void);

#endif // MAGNET_SKILL_H
