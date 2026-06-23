# Importing 3D Models from Tripo

## Export settings in Tripo

1. Generate your character/enemy/weapon in Tripo AI
2. Click **Export** → choose **GLB** format (GLTF Binary)
3. Enable **Rigged** if you want skeletal animation
4. Enable **Animations** and select: Idle, Walk, Run, Attack, Die
   - The engine maps animation index by order: [0]=Idle [1]=Walk [2]=Run [3]=Attack1 [4]=Attack2 [5]=Die [6]=Hit

## Placing models in the project

```
android/app/src/main/assets/models/
├── warrior.glb      ← local player
├── enemy.glb        ← enemy (re-used for all enemy types for now)
└── sword.glb        ← weapon (attached at runtime to warrior's hand bone)
```

Drop your exported `.glb` files here before building.

## Animation index contract

The engine loads animations in the order they appear in the GLB file.
When exporting from Tripo/Mixamo, name animations in this order:

| Index | Name     | Notes                  |
|-------|----------|------------------------|
| 0     | Idle     | Loop                   |
| 1     | Walk     | Loop                   |
| 2     | Run      | Loop                   |
| 3     | Attack1  | One-shot, ~1s          |
| 4     | Attack2  | One-shot (heavy swing) |
| 5     | Die      | One-shot               |
| 6     | Hit      | One-shot (flinch)      |

If your model has fewer animations, unused indices fall back to index 0 (Idle).

## Skinning

The GLTF loader supports up to **64 joints** and **4 weights per vertex**.
Tripo's rigged export is compatible out of the box.

## Texture notes

- Albedo / Base Color map is auto-loaded
- Normal map is auto-loaded
- Metallic-Roughness map is auto-loaded
- All textures are uploaded as mipmapped GL_TEXTURE_2D

## Sword attachment (future)

Weapon attachment to the `hand_R` bone will be automatic once the warrior model
includes a bone named `hand_R` (or `RightHand`) in its skeleton.
The engine already has `World::buildDrawCalls` hooks for weapon draw calls.
