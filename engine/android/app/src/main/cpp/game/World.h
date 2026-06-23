#pragma once
#include "Warrior.h"
#include "Enemy.h"
#include "../engine/Terrain.h"
#include "../engine/Renderer.h"
#include "../engine/Camera.h"
#include "../engine/GLTFLoader.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

struct RemotePlayer {
    uint32_t id;
    std::string name;
    Vec3  position;
    float yaw;
    int   hp, maxHp;
    int   animIndex;
    float animTime;
};

struct DamageNumber {
    Vec3  worldPos;
    int   value;
    float lifetime;
    float maxLifetime = 1.2f;
};

class World {
public:
    bool init(AAssetManager* assets);
    void update(float dt, const InputState& input);
    void render(Renderer& renderer);
    void destroy();

    // Local player
    Warrior localPlayer;

    // Enemies
    std::vector<std::unique_ptr<Enemy>> enemies;

    // Other players (from network)
    std::unordered_map<uint32_t, RemotePlayer> remotePlayers;

    Camera  camera;
    Terrain terrain;

    // Events
    std::function<void(int damage, Vec3 pos)> onDamageDealt;
    std::function<void(int xp)>               onXpGained;
    std::function<void(int level)>            onLevelUp;

    // Network callbacks
    void onNetworkPlayerJoin(const RemotePlayer& rp);
    void onNetworkPlayerLeave(uint32_t id);
    void onNetworkPlayerUpdate(const RemotePlayer& rp);
    void onNetworkEnemyUpdate(uint32_t id, Vec3 pos, int hp);

    std::vector<DamageNumber> damageNumbers;

    float worldTime = 0.f;

private:
    AAssetManager* assetMgr = nullptr;

    std::unordered_map<std::string, Model> models;

    bool loadModel(const std::string& name, const std::string& assetPath);
    void spawnEnemies();
    void updateCombat(float dt);
    void updateCamera(float dt);
    void clampToTerrain(Entity& e);
    void buildDrawCalls(std::vector<DrawCall>& out) const;
};
