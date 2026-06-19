#ifndef GAMEPLAY_H
#define GAMEPLAY_H

struct GameState;

// Cập nhật trạng thái gameplay màn chơi chính
void UpdateGameplay(struct GameState *gs, float deltaTime);

// Sinh lựa chọn kỹ năng sau khi diệt boss
void GenerateSkillChoices(struct GameState *gs);

// Chuyển sang biome tiếp theo sau khi diệt boss
void AdvanceToNextBiome(struct GameState *gs);

#endif // GAMEPLAY_H
