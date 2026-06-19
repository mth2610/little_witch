#ifndef COLLISION_H
#define COLLISION_H

struct GameState;

// Cập nhật toàn bộ các va chạm trong game loop gameplay
void CheckCollisions(struct GameState *gs);

#endif // COLLISION_H
