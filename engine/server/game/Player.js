'use strict';

let nextId = 1;

class Player {
    constructor(ws, name) {
        this.id        = nextId++;
        this.ws        = ws;
        this.name      = name || `Warrior_${this.id}`;
        this.x         = 64;   // spawn center of 128x128 terrain
        this.y         = 0;
        this.z         = 64;
        this.yaw       = 0;
        this.hp        = 150;
        this.maxHp     = 150;
        this.level     = 1;
        this.xp        = 0;
        this.anim      = 0;
        this.animTime  = 0;
        this.lastUpdate= Date.now();
        this.alive     = true;
    }

    toJSON() {
        return {
            id:       this.id,
            name:     this.name,
            x:        this.x,
            y:        this.y,
            z:        this.z,
            yaw:      this.yaw,
            hp:       this.hp,
            maxHp:    this.maxHp,
            level:    this.level,
            anim:     this.anim,
            animTime: this.animTime
        };
    }

    send(obj) {
        if(this.ws.readyState === 1 /* OPEN */) {
            try { this.ws.send(JSON.stringify(obj)); }
            catch(_) {}
        }
    }
}

module.exports = Player;
