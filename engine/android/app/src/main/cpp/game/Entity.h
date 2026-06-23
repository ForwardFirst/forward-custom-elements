#pragma once
#include "../math/Vec3.h"
#include "../math/Quaternion.h"
#include "../math/Mat4.h"
#include <string>
#include <cstdint>

enum class EntityType { Warrior, Enemy, Projectile, Item };
enum class EntityState { Idle, Walking, Running, Attacking, Dying, Dead };

struct Stats {
    int   hp      = 100;
    int   maxHp   = 100;
    int   attack  = 15;
    int   defense = 5;
    int   level   = 1;
    float speed   = 4.f;
    float attackRange = 2.2f;
    float attackCooldown = 1.2f;   // seconds between attacks
};

class Entity {
public:
    uint32_t    id         = 0;
    std::string name;
    EntityType  type       = EntityType::Enemy;
    EntityState state      = EntityState::Idle;
    Vec3        position   = {};
    Vec3        velocity   = {};
    float       yaw        = 0.f;   // facing direction
    float       radius     = 0.5f;  // collision radius
    bool        alive      = true;

    Stats       stats;

    // Animation
    int   currentAnim  = 0;   // index into model's animation list
    float animTime     = 0.f;

    Mat4 modelMatrix() const {
        return Mat4::Translation(position)
             * Mat4::RotationY(yaw)
             * Mat4::Scale(1,1,1);
    }

    virtual void update(float dt) {
        position += velocity * dt;
        animTime += dt;
    }

    bool isDead() const { return !alive || stats.hp <= 0; }
    void takeDamage(int dmg) {
        stats.hp -= std::max(0, dmg - stats.defense);
        if(stats.hp <= 0) { stats.hp=0; alive=false; state=EntityState::Dying; }
    }
};
