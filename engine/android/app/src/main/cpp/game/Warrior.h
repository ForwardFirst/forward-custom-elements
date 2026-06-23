#pragma once
#include "Entity.h"
#include "../math/Vec3.h"

// Animation indices (matches GLTF animation list from Tripo/Mixamo rig)
namespace WarriorAnim {
    constexpr int Idle    = 0;
    constexpr int Walk    = 1;
    constexpr int Run     = 2;
    constexpr int Attack1 = 3;
    constexpr int Attack2 = 4;
    constexpr int Die     = 5;
    constexpr int Hit     = 6;
}

struct InputState {
    Vec2  moveStick  = {};    // normalized -1..1
    bool  attackBtn  = false;
    bool  jumpBtn    = false;
    bool  dodgeBtn   = false;
    float cameraYaw  = 0.f;
    float cameraPitch= 0.f;
};

class Warrior : public Entity {
public:
    Warrior();

    void update(float dt) override;
    void applyInput(const InputState& input, float dt);

    // Combat
    float attackTimer  = 0.f;
    int   xp           = 0;
    int   nextLevelXp  = 100;
    float dodgeTimer   = 0.f;
    float dodgeDur     = 0.4f;
    float dodgeSpeed   = 9.f;
    Vec3  dodgeDir     = {};

    bool  isAttacking  = false;
    bool  attackHitFrame = false;  // true on the frame damage should be applied

    void  gainXp(int amount);
    void  levelUp();

    // Network id assigned by server
    uint32_t serverId = 0;

private:
    float cameraYaw = 0.f;
};
