#include "Enemy.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

static float randF(float lo, float hi) {
    return lo + (float)rand()/(float)RAND_MAX * (hi-lo);
}

static float dist2D(const Vec3& a, const Vec3& b) {
    float dx=a.x-b.x, dz=a.z-b.z;
    return std::sqrt(dx*dx+dz*dz);
}

bool Enemy::canAttack(const Vec3& targetPos) const {
    return dist2D(position, targetPos) <= attackRange;
}

void Enemy::doAttack(Entity& target) {
    target.takeDamage(stats.attack);
}

void Enemy::update(float dt, const Vec3& playerPos, bool playerAlive) {
    if(isDead()) { state=EntityState::Dead; currentAnim=EnemyAnim::Die; return; }

    attackTimer  = std::max(0.f, attackTimer-dt);
    roamTimer    = std::max(0.f, roamTimer-dt);
    alertTimer   = std::max(0.f, alertTimer-dt);
    animTime    += dt;

    float dToPlayer = dist2D(position, playerPos);

    switch(ai) {
    case EnemyAI::Roaming: {
        state      = EntityState::Walking;
        currentAnim= EnemyAnim::Walk;
        // Patrol to random nearby point
        if(roamTimer <= 0.f || dist2D(position,roamTarget)<0.5f) {
            roamTarget  = spawnPos + Vec3{randF(-6,6),0,randF(-6,6)};
            roamTimer   = randF(3.f,8.f);
        }
        Vec3 dir = (roamTarget-position);
        dir.y=0;
        float d=dir.length();
        if(d>0.1f){ dir=dir*(1.f/d); velocity=dir*(stats.speed*0.4f); yaw=std::atan2(dir.x,dir.z); }
        else velocity={};
        // Aggro check
        if(playerAlive && dToPlayer < aggroRange) {
            ai=EnemyAI::Alerted; alertTimer=0.6f; velocity={};
            state=EntityState::Idle; currentAnim=EnemyAnim::Alert;
        }
        break;
    }
    case EnemyAI::Alerted:
        state=EntityState::Idle; currentAnim=EnemyAnim::Alert; velocity={};
        if(alertTimer<=0.f) ai=EnemyAI::Chasing;
        break;

    case EnemyAI::Chasing: {
        Vec3 dir = playerPos-position;
        dir.y=0;
        float d=dir.length();
        if(d>0.1f){ dir=dir*(1.f/d); velocity=dir*stats.speed; yaw=std::atan2(dir.x,dir.z); }
        state=EntityState::Running; currentAnim=EnemyAnim::Walk;
        // Deaggro
        if(!playerAlive || dToPlayer > deaggroRange) { ai=EnemyAI::Returning; velocity={}; }
        // Attack range
        if(dToPlayer <= attackRange) { ai=EnemyAI::Attacking; velocity={}; }
        break;
    }
    case EnemyAI::Attacking: {
        velocity={};
        state=EntityState::Attacking; currentAnim=EnemyAnim::Attack;
        // Face player
        Vec3 dir=playerPos-position; dir.y=0;
        if(dir.length()>0.1f) yaw=std::atan2(dir.x,dir.z);

        if(attackTimer<=0.f){
            attackTimer=stats.attackCooldown;
            // Damage applied via World/Combat
        }
        if(dToPlayer > attackRange+0.5f) ai=EnemyAI::Chasing;
        if(!playerAlive) ai=EnemyAI::Roaming;
        break;
    }
    case EnemyAI::Returning: {
        Vec3 dir=spawnPos-position; dir.y=0;
        float d=dir.length();
        if(d<0.5f){ ai=EnemyAI::Roaming; velocity={}; break; }
        dir=dir*(1.f/d); velocity=dir*(stats.speed*0.6f); yaw=std::atan2(dir.x,dir.z);
        state=EntityState::Walking; currentAnim=EnemyAnim::Walk;
        // Aggro again if player gets close
        if(playerAlive && dToPlayer < aggroRange*0.7f) ai=EnemyAI::Chasing;
        break;
    }
    case EnemyAI::Dead: break;
    }

    Entity::update(dt);
}
