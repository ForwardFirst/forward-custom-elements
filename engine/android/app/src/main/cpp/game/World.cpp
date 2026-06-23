#include "World.h"
#include <android/log.h>
#include <android/asset_manager.h>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#define TAG  "World"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)

static uint32_t gNextId = 1;

bool World::loadModel(const std::string& name, const std::string& assetPath) {
    AAsset* asset = AAssetManager_open(assetMgr, assetPath.c_str(), AASSET_MODE_BUFFER);
    if(!asset){ LOGE("Cannot open asset: %s", assetPath.c_str()); return false; }
    size_t size = AAsset_getLength(asset);
    std::vector<uint8_t> buf(size);
    AAsset_read(asset, buf.data(), size);
    AAsset_close(asset);

    Model m;
    if(!GLTFLoader::loadFromMemory(buf.data(), size, "", m)) return false;
    models[name] = std::move(m);
    LOGI("Loaded model '%s'", name.c_str());
    return true;
}

bool World::init(AAssetManager* assets) {
    assetMgr = assets;
    srand(42);

    terrain.generate();

    // Spawn warrior at center
    localPlayer.id       = gNextId++;
    localPlayer.position = {terrain.gridW*0.5f*terrain.cellSize,
                            0,
                            terrain.gridH*0.5f*terrain.cellSize};
    localPlayer.position.y = terrain.groundY(localPlayer.position.x, localPlayer.position.z) + 0.1f;

    camera.target  = localPlayer.position + Vec3{0,1,0};
    camera.yaw     = 0.f;
    camera.pitch   = 0.4f;
    camera.distance= 5.f;

    spawnEnemies();

    // Try to load models (will silently skip if not found - placeholders used)
    loadModel("warrior", "models/warrior.glb");
    loadModel("enemy",   "models/enemy.glb");
    loadModel("sword",   "models/sword.glb");

    LOGI("World initialised");
    return true;
}

void World::spawnEnemies() {
    float cx = terrain.gridW*0.5f*terrain.cellSize;
    float cz = terrain.gridH*0.5f*terrain.cellSize;

    for(int i=0;i<20;i++) {
        auto e = std::make_unique<Enemy>();
        e->id = gNextId++;
        float angle = (float)i/20.f * 6.2832f;
        float r     = 10.f + (float)(rand()%15);
        e->position = {cx + std::cos(angle)*r,
                       0,
                       cz + std::sin(angle)*r};
        e->position.y = terrain.groundY(e->position.x, e->position.z) + 0.1f;
        e->spawnPos   = e->position;
        e->stats.hp   = e->stats.maxHp = 60 + rand()%40;
        e->stats.attack = 8 + rand()%6;
        e->stats.speed  = 2.5f + (float)(rand()%20)/10.f;
        e->xpReward     = 20 + rand()%30;
        enemies.push_back(std::move(e));
    }
}

void World::updateCombat(float dt) {
    if(localPlayer.isDead()) return;

    // Warrior attacking enemies
    if(localPlayer.attackHitFrame) {
        for(auto& e : enemies) {
            if(e->isDead()) continue;
            float dx = e->position.x - localPlayer.position.x;
            float dz = e->position.z - localPlayer.position.z;
            float dist = std::sqrt(dx*dx+dz*dz);
            if(dist <= localPlayer.stats.attackRange) {
                int dmg = std::max(1, localPlayer.stats.attack - e->stats.defense/2);
                e->takeDamage(dmg);
                if(onDamageDealt) onDamageDealt(dmg, e->position);

                DamageNumber dn;
                dn.worldPos  = e->position + Vec3{0,2,0};
                dn.value     = dmg;
                dn.lifetime  = dn.maxLifetime;
                damageNumbers.push_back(dn);

                if(e->isDead()) {
                    localPlayer.gainXp(e->xpReward);
                    if(onXpGained) onXpGained(e->xpReward);
                }
            }
        }
        localPlayer.attackHitFrame = false;
    }

    // Enemies attacking warrior
    for(auto& e : enemies) {
        if(e->isDead() || e->ai != EnemyAI::Attacking) continue;
        if(e->attackTimer <= 0.f) {
            if(e->canAttack(localPlayer.position)) {
                int dmg = std::max(1, e->stats.attack - localPlayer.stats.defense/2);
                localPlayer.takeDamage(dmg);
                e->attackTimer = e->stats.attackCooldown;
            }
        }
    }

    // Update damage numbers
    for(auto& dn : damageNumbers) dn.lifetime -= dt;
    damageNumbers.erase(
        std::remove_if(damageNumbers.begin(), damageNumbers.end(),
                       [](const DamageNumber& d){ return d.lifetime<=0; }),
        damageNumbers.end());
}

void World::clampToTerrain(Entity& e) {
    float ground = terrain.groundY(e.position.x, e.position.z);
    if(e.position.y < ground + 0.1f) {
        e.position.y = ground + 0.1f;
        if(e.velocity.y < 0) e.velocity.y = 0;
    } else {
        e.velocity.y -= 9.8f * 0.016f; // gravity
    }
    // Terrain boundary clamp
    float maxX = terrain.gridW * terrain.cellSize - 1.f;
    float maxZ = terrain.gridH * terrain.cellSize - 1.f;
    e.position.x = std::clamp(e.position.x, 1.f, maxX);
    e.position.z = std::clamp(e.position.z, 1.f, maxZ);
}

void World::update(float dt, const InputState& input) {
    worldTime += dt;

    // Player
    localPlayer.applyInput(input, dt);
    localPlayer.update(dt);
    clampToTerrain(localPlayer);

    // Camera follows player
    updateCamera(dt);

    // Enemies
    for(auto& e : enemies)
        e->update(dt, localPlayer.position, !localPlayer.isDead());
    for(auto& e : enemies) clampToTerrain(*e);

    // Remove dead enemies (after delay for death animation)
    for(auto& e : enemies) {
        if(e->isDead() && e->animTime > 2.f) e->alive = false;
    }

    updateCombat(dt);
}

void World::updateCamera(float dt) {
    // Smooth follow
    Vec3 targetPos = localPlayer.position + Vec3{0,1.2f,0};
    camera.target  = Vec3::lerp(camera.target, targetPos, dt*8.f);

    if(camera.mode == CameraMode::ThirdPerson) {
        // Camera yaw matches input camera yaw (set from touch drag in Kotlin)
    } else {
        camera.setFirstPersonPose(
            localPlayer.position + Vec3{0,1.6f,0},
            localPlayer.yaw + 3.14159f,
            0.f);
    }
}

void World::buildDrawCalls(std::vector<DrawCall>& out) const {
    // Local player
    if(models.count("warrior")) {
        DrawCall dc;
        dc.mesh       = &models.at("warrior").mesh;
        dc.modelMatrix= localPlayer.modelMatrix();
        dc.animIndex  = localPlayer.currentAnim;
        dc.animTime   = localPlayer.animTime;
        const Model& m = models.at("warrior");
        if(m.hasSkin) m.computeSkinMatrices(dc.animIndex, dc.animTime, dc.jointMatrices);
        out.push_back(dc);
    }

    // Enemies
    if(models.count("enemy")) {
        for(auto& e : enemies) {
            if(e->state == EntityState::Dead && e->animTime > 2.5f) continue;
            DrawCall dc;
            dc.mesh       = &models.at("enemy").mesh;
            dc.modelMatrix= e->modelMatrix();
            dc.animIndex  = e->currentAnim;
            dc.animTime   = e->animTime;
            const Model& m = models.at("enemy");
            if(m.hasSkin) m.computeSkinMatrices(dc.animIndex, dc.animTime, dc.jointMatrices);
            out.push_back(dc);
        }
    }

    // Remote players
    if(models.count("warrior")) {
        for(auto& [id, rp] : remotePlayers) {
            DrawCall dc;
            dc.mesh = &models.at("warrior").mesh;
            Mat4 mm = Mat4::Translation(rp.position) * Mat4::RotationY(rp.yaw);
            dc.modelMatrix= mm;
            dc.animIndex  = rp.animIndex;
            dc.animTime   = rp.animTime;
            const Model& m = models.at("warrior");
            if(m.hasSkin) m.computeSkinMatrices(dc.animIndex, dc.animTime, dc.jointMatrices);
            out.push_back(dc);
        }
    }
}

void World::render(Renderer& renderer) {
    renderer.beginFrame();

    renderer.drawTerrain(terrain, camera);

    std::vector<DrawCall> dcs;
    buildDrawCalls(dcs);
    renderer.drawMeshes(dcs, camera);

    renderer.endFrame();
}

void World::onNetworkPlayerJoin(const RemotePlayer& rp) {
    remotePlayers[rp.id] = rp;
}
void World::onNetworkPlayerLeave(uint32_t id) {
    remotePlayers.erase(id);
}
void World::onNetworkPlayerUpdate(const RemotePlayer& rp) {
    remotePlayers[rp.id] = rp;  // upsert — handles both join and update
}
void World::onNetworkEnemyUpdate(uint32_t id, Vec3 pos, int hp) {
    for(auto& e : enemies)
        if(e->id == id) { e->position=pos; e->stats.hp=hp; break; }
}
void World::destroy() {
    terrain.destroy();
    for(auto& [k,m] : models) m.destroy();
    models.clear();
    enemies.clear();
}
