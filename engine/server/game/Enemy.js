'use strict';

let nextEnemyId = 10000;

const AI = { ROAMING: 0, CHASING: 1, ATTACKING: 2, RETURNING: 3, DEAD: 4 };

class Enemy {
    constructor(spawnX, spawnZ) {
        this.id           = nextEnemyId++;
        this.x            = spawnX;
        this.y            = 0;
        this.z            = spawnZ;
        this.spawnX       = spawnX;
        this.spawnZ       = spawnZ;
        this.yaw          = 0;
        this.hp           = 60 + Math.floor(Math.random() * 40);
        this.maxHp        = this.hp;
        this.attack       = 8 + Math.floor(Math.random() * 6);
        this.defense      = 2;
        this.speed        = 2.5 + Math.random() * 2;
        this.aggroRange   = 12;
        this.attackRange  = 2.2;
        this.attackTimer  = 0;
        this.attackCooldown = 1.5;
        this.roamTimer    = Math.random() * 4;
        this.roamX        = spawnX;
        this.roamZ        = spawnZ;
        this.ai           = AI.ROAMING;
        this.xpReward     = 20 + Math.floor(Math.random() * 30);
        this.targetId     = -1;
        this.respawnTimer = 0;
        this.dead         = false;
    }

    dist2D(x, z) {
        const dx = this.x - x, dz = this.z - z;
        return Math.sqrt(dx*dx + dz*dz);
    }

    update(dt, players) {
        if(this.dead) {
            this.respawnTimer -= dt;
            if(this.respawnTimer <= 0) {
                this.x = this.spawnX; this.z = this.spawnZ;
                this.hp = this.maxHp; this.dead = false;
                this.ai = AI.ROAMING; this.targetId = -1;
            }
            return;
        }

        this.attackTimer = Math.max(0, this.attackTimer - dt);
        this.roamTimer   = Math.max(0, this.roamTimer - dt);

        // Find nearest player
        let nearest = null, nearestDist = Infinity;
        for(const [, p] of players) {
            if(!p.alive) continue;
            const d = this.dist2D(p.x, p.z);
            if(d < nearestDist) { nearestDist=d; nearest=p; }
        }

        switch(this.ai) {
        case AI.ROAMING: {
            if(this.roamTimer <= 0 || this.dist2D(this.roamX, this.roamZ) < 0.5) {
                this.roamX = this.spawnX + (Math.random()-0.5)*12;
                this.roamZ = this.spawnZ + (Math.random()-0.5)*12;
                this.roamTimer = 3 + Math.random()*5;
            }
            const dx=this.roamX-this.x, dz=this.roamZ-this.z, d=Math.sqrt(dx*dx+dz*dz);
            if(d>0.3){ this.x+=dx/d*this.speed*0.4*dt; this.z+=dz/d*this.speed*0.4*dt; this.yaw=Math.atan2(dx,dz); }
            if(nearest && nearestDist < this.aggroRange) { this.ai=AI.CHASING; this.targetId=nearest.id; }
            break;
        }
        case AI.CHASING: {
            const target = nearest;
            if(!target || nearestDist > this.aggroRange*1.5) { this.ai=AI.RETURNING; break; }
            const dx=target.x-this.x, dz=target.z-this.z, d=Math.sqrt(dx*dx+dz*dz);
            if(d>0.1){ this.x+=dx/d*this.speed*dt; this.z+=dz/d*this.speed*dt; this.yaw=Math.atan2(dx,dz); }
            if(nearestDist <= this.attackRange) this.ai=AI.ATTACKING;
            break;
        }
        case AI.ATTACKING: {
            const target = nearest;
            if(!target || nearestDist > this.attackRange+0.5) { this.ai=AI.CHASING; break; }
            const dx=target.x-this.x, dz=target.z-this.z;
            if(Math.sqrt(dx*dx+dz*dz)>0.1) this.yaw=Math.atan2(dx,dz);
            if(this.attackTimer<=0) {
                this.attackTimer = this.attackCooldown;
                const dmg = Math.max(1, this.attack - Math.floor(target.hp/20));
                target.hp = Math.max(0, target.hp - dmg);
                if(target.hp<=0) { target.alive=false; this.ai=AI.ROAMING; }
            }
            break;
        }
        case AI.RETURNING: {
            const dx=this.spawnX-this.x, dz=this.spawnZ-this.z, d=Math.sqrt(dx*dx+dz*dz);
            if(d<0.5){ this.ai=AI.ROAMING; this.hp=this.maxHp; break; }
            this.x+=dx/d*this.speed*0.6*dt; this.z+=dz/d*this.speed*0.6*dt;
            if(nearest && nearestDist < this.aggroRange*0.7) { this.ai=AI.CHASING; }
            break;
        }
        }
    }

    takeDamage(dmg, attackerId) {
        if(this.dead) return 0;
        const actual = Math.max(1, dmg - this.defense);
        this.hp -= actual;
        if(this.hp <= 0) {
            this.hp = 0; this.dead = true; this.respawnTimer = 30;
            this.ai = AI.DEAD; this.targetId = -1;
        } else {
            this.ai = AI.CHASING; this.targetId = attackerId;
        }
        return actual;
    }

    toJSON() {
        return { id:this.id, x:this.x, y:this.y, z:this.z, yaw:this.yaw,
                 hp:this.hp, maxHp:this.maxHp, dead:this.dead, ai:this.ai };
    }
}

module.exports = { Enemy, AI };
