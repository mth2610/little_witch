#ifndef WITCH_H
#define WITCH_H

#include "raylib.h"
#include "config.h"



typedef enum SkillType {
    SKILL_NONE          = 0,
    SKILL_SHURIKEN      = 1, // Kim (Golden Shuriken)
    SKILL_POISON_CLOUD  = 2, // Mộc (Poison Cloud)
    SKILL_ICE_BLAST     = 3, // Thủy (Ice Blast)
    SKILL_FIREBALL      = 4, // Hỏa (Fireball)
    SKILL_LIGHTNING     = 5, // Thổ (Lightning & Tornado)
    SKILL_MAGNET        = 6, // Nam châm chủ động
    SKILL_SHIELD        = 7, // Khiên chủ động
    SKILL_COUNT         = 8
} SkillType;

typedef enum UpgradeType {
    UPGRADE_MAX_LIVES   = 0,
    UPGRADE_SPEED       = 1,
    UPGRADE_LUCK        = 2,
    UPGRADE_MAGNET_BASE = 3,
    UPGRADE_COUNT       = 4
} UpgradeType;

typedef struct PermanentUpgrades {
    int level[UPGRADE_COUNT];
} PermanentUpgrades;

typedef enum WitchAnimState {
    WITCH_STATE_FLY = 0,
    WITCH_STATE_ATTACK_HAND = 1,
    WITCH_STATE_ATTACK_WEAPON = 2,
    WITCH_STATE_DEFENSE = 3
} WitchAnimState;

typedef struct Witch {
    Vector2 position;
    Vector2 targetPosition;
    Vector2 velocity;
    float   collisionRadius;
    int     lives;
    bool    isInvincible;
    float   invincibleTimer;
    bool    hasRevived;
    int     gold;
    int     score;
    bool    activeSkills[SKILL_COUNT];
    int     skillLevels[SKILL_COUNT];
    float   skillCooldown[SKILL_COUNT];
    float   skillActiveTimer[SKILL_COUNT];
    float   manaShieldHealth;
    PermanentUpgrades upgrades;
    float   animTimer;
    int     animFrame;
    bool    facingRight;
    float   swipeBoostTimer; // Hết hạn boost tốc độ sau vuốt màn hình
    WitchAnimState animState;
    float   stateTimer;
    Vector2 prevVirtualPos;         // Vị trí touch/chuột frame trước (phát hiện swipe)
    bool    prevInputPressed;       // Frame trước có input cảm ứng không
    float   keyboardAttackCooldown; // Hồi chiêu tấn công bàn phím (SPACE)
    Vector2 positionHistory[4];     // Lịch sử vị trí vẽ bóng mờ (Afterimages)

    int     effectiveKimStars;      // Số sao hệ Kim hiệu dụng
    int     effectiveMocStars;      // Số sao hệ Mộc hiệu dụng
    int     effectiveThuyStars;     // Số sao hệ Thủy hiệu dụng
    int     effectiveHoaStars;      // Số sao hệ Hỏa hiệu dụng
    int     effectiveThoStars;      // Số sao hệ Thổ hiệu dụng
} Witch;

void  InitWitch(Witch *witch, PermanentUpgrades upgrades);
void  UpdateWitch(Witch *witch, float deltaTime);
void  DrawWitch(const Witch *witch);
bool  WitchTakeDamage(Witch *witch);
bool  WitchGrantSkill(Witch *witch, SkillType skill);
float WitchGetEffectiveSpeed(const Witch *witch);

#endif // WITCH_H
