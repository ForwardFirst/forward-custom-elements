#include "Warrior.h"
#include <cmath>
#include <algorithm>

Warrior::Warrior() {
    type          = EntityType::Warrior;
    name          = "Warrior";
    radius        = 0.45f;
    stats.hp      = 150;
    stats.maxHp   = 150;
    stats.attack  = 20;
    stats.defense = 8;
    stats.speed   = 5.f;
    stats.attackRange    = 2.0f;
    stats.attackCooldown = 1.0f;
}

void Warrior::applyInput(const InputState& input, float dt) {
    if(isDead()) return;
    cameraYaw = input.cameraYaw;

    // Dodge roll
    if(input.dodgeBtn && dodgeTimer <= 0.f) {
        Vec2 dir = input.moveStick;
        if(dir.length() < 0.1f) dir = {0,1};  // forward dodge
        dodgeDir  = Vec3{dir.x, 0, dir.y}.normalized();
        dodgeTimer = dodgeDur;
    }

    if(dodgeTimer > 0.f) {
        dodgeTimer -= dt;
        // Rotate dodge direction by camera yaw
        float cy = std::cos(cameraYaw), sy = std::sin(cameraYaw);
        Vec3  wd = {cy*dodgeDir.x - sy*dodgeDir.z, 0,
                    sy*dodgeDir.x + cy*dodgeDir.z};
        velocity = wd * dodgeSpeed;
        state    = EntityState::Running;
        return;
    }

    // Attack
    attackTimer = std::max(0.f, attackTimer - dt);
    attackHitFrame = false;
    if(input.attackBtn && attackTimer <= 0.f) {
        attackTimer  = stats.attackCooldown;
        isAttacking  = true;
        state        = EntityState::Attacking;
        currentAnim  = WarriorAnim::Attack1;
        animTime     = 0.f;
        // Hit frame at ~40% of attack duration
        attackHitFrame = true;
    } else if(isAttacking && attackTimer <= stats.attackCooldown * 0.5f) {
        isAttacking = false;
    }

    if(isAttacking) { velocity = {}; return; }

    // Movement (move relative to camera)
    Vec2 stick = input.moveStick;
    float mag  = stick.length();
    if(mag > 1.f) { stick = stick * (1.f/mag); mag=1.f; }

    if(mag > 0.05f) {
        float cy = std::cos(cameraYaw), sy = std::sin(cameraYaw);
        Vec3 moveDir = {
            cy*stick.x - sy*stick.y,
            0,
            sy*stick.x + cy*stick.y
        };
        moveDir.normalize();
        float speed = mag * stats.speed * (mag > 0.7f ? 1.f : 0.6f);
        velocity    = moveDir * speed;
        yaw         = std::atan2(moveDir.x, moveDir.z);
        state       = mag > 0.7f ? EntityState::Running : EntityState::Walking;
        currentAnim = mag > 0.7f ? WarriorAnim::Run : WarriorAnim::Walk;
    } else {
        velocity    = {};
        state       = EntityState::Idle;
        currentAnim = WarriorAnim::Idle;
    }
}

void Warrior::update(float dt) {
    Entity::update(dt);
    // Gravity / ground clamping handled by World
}

void Warrior::gainXp(int amount) {
    xp += amount;
    if(xp >= nextLevelXp) levelUp();
}

void Warrior::levelUp() {
    stats.level++;
    xp -= nextLevelXp;
    nextLevelXp = (int)(nextLevelXp * 1.5f);
    stats.maxHp   += 20;
    stats.hp       = stats.maxHp;
    stats.attack  += 5;
    stats.defense += 2;
}
