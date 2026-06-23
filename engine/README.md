# Forward MMORPG Engine

A custom 3D MMORPG engine for Android, built from scratch with OpenGL ES 3.0 and a Node.js multiplayer server.

## Architecture

```
engine/
├── android/                  ← Android app (Kotlin + C++ NDK)
│   └── app/src/main/
│       ├── cpp/              ← C++ game engine
│       │   ├── math/         ← Vec3, Mat4, Quaternion (header-only)
│       │   ├── engine/       ← Renderer, Shader, Mesh, Camera, Terrain, GLTFLoader
│       │   ├── game/         ← Warrior, Enemy, World, Entity
│       │   ├── third_party/  ← tiny_gltf, stb_image, nlohmann/json
│       │   └── GameBridge.cpp ← JNI bridge to Kotlin
│       └── kotlin/           ← Android UI layer
│           ├── MainActivity.kt
│           ├── GameSurfaceView.kt  ← GLSurfaceView + touch camera
│           ├── HUDView.kt    ← Joystick, HP bar, XP bar, buttons
│           ├── NetworkManager.kt  ← WebSocket client
│           └── GameBridge.kt ← JNI declarations
└── server/                   ← Node.js multiplayer server
    ├── server.js             ← WebSocket server
    └── game/
        ├── GameState.js      ← Game loop, enemy AI tick
        ├── Player.js         ← Player state
        └── Enemy.js          ← Server-side enemy AI
```

## Build — Android

### Prerequisites
- Android Studio Hedgehog (2023.1) or newer
- NDK 26+
- CMake 3.22+
- API 26+ device or emulator

### Steps

```bash
cd engine/android
./gradlew assembleDebug
```

Or open `engine/android/` in Android Studio and press Run.

### First-time setup

The third-party headers (tiny_gltf, stb_image, nlohmann/json) are already included in the repo.
If you need to refresh them:

```bash
cd android/app/src/main/cpp/third_party
bash fetch_deps.sh
```

## Build — Server

```bash
cd engine/server
npm install
npm start
```

Server runs on `ws://0.0.0.0:3000`.

For production, set `PORT` env var and put behind nginx with TLS.

## Adding 3D Models

See [MODELS.md](MODELS.md) for how to import from Tripo AI.

## Gameplay

| Control        | Action                          |
|---------------|---------------------------------|
| Left joystick  | Move (camera-relative)          |
| Right drag     | Rotate camera                   |
| Pinch          | Zoom in/out                     |
| ATK button     | Sword attack (melee)            |
| DODGE button   | Dodge roll                      |
| CAM button     | Toggle 3rd / 1st person         |

## Engine features

- **Renderer**: OpenGL ES 3.0, PBR lighting (Blinn-Phong + GGX specular)
- **Terrain**: Procedural heightmap with FBM noise, 128×128 grid
- **Models**: GLTF 2.0 / GLB loader (positions, normals, UVs, skinning, animations)
- **Skeletal animation**: Linear-interpolated keyframe playback, up to 64 joints
- **Camera**: Third-person orbit + first-person toggle
- **Combat**: Warrior melee, enemy patrol/aggro/attack AI, XP and leveling
- **Networking**: WebSocket (OkHttp client ↔ ws Node.js server), ~12Hz player sync
- **Class**: Warrior (sword) — designed to expand to multiple classes

## Roadmap

- [ ] Sword weapon mesh attached to hand bone
- [ ] Spell / ranged attack system
- [ ] Loot drops and inventory
- [ ] Multiple enemy types (boss, ranged, mage)
- [ ] Chat system
- [ ] World zones / dungeons
- [ ] Guild system
- [ ] Cloud server deploy (Fly.io / Railway)
