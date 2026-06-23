#pragma once
#include "Entity.h"

namespace EnemyAnim {
    constexpr int Idle    = 0;
    constexpr int Walk    = 1;
    constexpr int Attack  = 2;
    constexpr int Die     = 3;
    constexpr int Alert   = 4;
}

enum class EnemyAI { Roaming, Alerted, Chasing, Attacking, Returning, Dead };

class Enemy : public Entity {
public:
    EnemyAI  ai           = EnemyAI::Roaming;
    Vec3     spawnPos     = {};
    Vec3     roamTarget   = {};
    float    aggroRange   = 12.f;
    float    deaggroRange = 20.f;
    float    attackRange  = 2.0f;
    float    attackTimer  = 0.f;
    float    roamTimer    = 0.f;
    float    alertTimer   = 0.f;
    uint32_t targetId     = 0;   // id of entity being attacked
    int      xpReward     = 25;
    int      lootChance   = 30;  // %

    Enemy() { type=EntityType::Enemy; radius=0.5f; }

    void update(float dt, const Vec3& playerPos, bool playerAlive);

    bool canAttack(const Vec3& targetPos) const;
    void doAttack(Entity& target);
};
