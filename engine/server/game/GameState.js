'use strict';

const { Enemy } = require('./Enemy');

const TICK_RATE   = 20;      // server ticks per second
const TICK_MS     = 1000 / TICK_RATE;
const ENEMY_COUNT = 20;

class GameState {
    constructor(broadcast) {
        this.players  = new Map();   // id → Player
        this.enemies  = new Map();   // id → Enemy
        this.broadcast = broadcast;
        this._spawnEnemies();
        this._lastTick = Date.now();
        this._interval = setInterval(() => this._tick(), TICK_MS);
    }

    _spawnEnemies() {
        for(let i=0;i<ENEMY_COUNT;i++){
            const angle = (i/ENEMY_COUNT)*Math.PI*2;
            const r     = 10 + Math.random()*15;
            const e     = new Enemy(64+Math.cos(angle)*r, 64+Math.sin(angle)*r);
            this.enemies.set(e.id, e);
        }
    }

    addPlayer(player) {
        this.players.set(player.id, player);
        // Send existing state to new player
        player.send({ type:'auth', id:player.id });
        for(const [, p] of this.players) {
            if(p.id !== player.id) player.send({ type:'player_join', ...p.toJSON() });
        }
        // Announce join to others
        this.broadcast({ type:'player_join', ...player.toJSON() }, player.id);
    }

    removePlayer(playerId) {
        this.players.delete(playerId);
        this.broadcast({ type:'player_leave', id:playerId });
    }

    handleMessage(player, msg) {
        switch(msg.type) {
        case 'player_update':
            player.x        = msg.x        ?? player.x;
            player.y        = msg.y        ?? player.y;
            player.z        = msg.z        ?? player.z;
            player.yaw      = msg.yaw      ?? player.yaw;
            player.hp       = msg.hp       ?? player.hp;
            player.anim     = msg.anim     ?? player.anim;
            player.animTime = msg.animTime ?? player.animTime;
            player.alive    = player.hp > 0;
            player.lastUpdate = Date.now();
            // Relay to other players (broadcast)
            this.broadcast({ type:'player_update', ...player.toJSON() }, player.id);
            break;

        case 'attack_enemy': {
            const eid = msg.enemyId;
            const enemy = this.enemies.get(eid);
            if(!enemy) break;
            const dmg  = Math.max(1, (msg.damage||15));
            const actual = enemy.takeDamage(dmg, player.id);
            if(actual > 0) {
                // Broadcast enemy update
                this.broadcast({ type:'enemy_update', ...enemy.toJSON() });
                if(enemy.dead) {
                    // XP reward
                    player.xp = (player.xp||0) + enemy.xpReward;
                    player.send({ type:'xp_gain', xp:enemy.xpReward, totalXp:player.xp });
                }
            }
            break;
        }

        case 'ping':
            player.send({ type:'pong', t: msg.t });
            break;
        }
    }

    _tick() {
        const now = Date.now();
        const dt  = (now - this._lastTick) / 1000;
        this._lastTick = now;

        // Update enemies
        for(const [, e] of this.enemies) {
            e.update(dt, this.players);
        }

        // Broadcast enemy batch every 5 ticks (~4Hz) to save bandwidth
        this._enemyBroadcastCounter = (this._enemyBroadcastCounter||0)+1;
        if(this._enemyBroadcastCounter >= 5) {
            this._enemyBroadcastCounter = 0;
            const batch = [];
            for(const [, e] of this.enemies) batch.push(e.toJSON());
            this.broadcast({ type:'enemy_batch', enemies:batch });
        }
    }

    destroy() {
        clearInterval(this._interval);
    }
}

module.exports = GameState;
