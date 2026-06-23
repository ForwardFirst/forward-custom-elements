'use strict';

const { WebSocketServer } = require('ws');
const Player    = require('./game/Player');
const GameState = require('./game/GameState');

const PORT = process.env.PORT || 3000;

const wss = new WebSocketServer({ port: PORT });

// Broadcast to all connected clients, optionally excluding one id
function broadcast(obj, excludeId = -1) {
    const msg = JSON.stringify(obj);
    wss.clients.forEach(client => {
        if(client.readyState === 1 && client._playerId !== excludeId) {
            try { client.send(msg); } catch(_) {}
        }
    });
}

const gameState = new GameState(broadcast);

console.log(`[Server] Forward MMORPG server listening on ws://0.0.0.0:${PORT}`);

wss.on('connection', (ws, req) => {
    const ip = req.socket.remoteAddress;
    console.log(`[+] Connection from ${ip}`);

    let player = null;

    ws.on('message', rawData => {
        let msg;
        try { msg = JSON.parse(rawData); }
        catch(_) { return; }

        // First message must be auth
        if(!player) {
            if(msg.type !== 'auth') return;
            player = new Player(ws, msg.name || 'Warrior');
            ws._playerId = player.id;
            console.log(`[>] Player joined: ${player.name} (id=${player.id})`);
            gameState.addPlayer(player);
            return;
        }

        gameState.handleMessage(player, msg);
    });

    ws.on('close', () => {
        if(player) {
            console.log(`[-] Player left: ${player.name} (id=${player.id})`);
            gameState.removePlayer(player.id);
            player = null;
        }
    });

    ws.on('error', err => {
        console.error(`[!] WS error for player ${player?.id}:`, err.message);
    });
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n[Server] Shutting down...');
    gameState.destroy();
    wss.close(() => process.exit(0));
});
