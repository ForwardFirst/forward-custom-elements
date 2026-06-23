#pragma once
#include "Mesh.h"
#include "../math/Mat4.h"
#include "../math/Quaternion.h"
#include <string>
#include <vector>

// Skeletal joint
struct Joint {
    std::string name;
    int         parent = -1;
    Mat4        inverseBindMatrix;
    Vec3        localTranslation = {};
    Quaternion  localRotation    = {};
    Vec3        localScale       = {1,1,1};
};

// Single channel of animation keyframes
struct AnimChannel {
    int    jointIndex = 0;
    enum class Path { Translation, Rotation, Scale } path;
    std::vector<float>     times;
    std::vector<float>     values;   // 3 or 4 floats per key
    enum class Interp { Linear, Step, CubicSpline } interp = Interp::Linear;
};

struct Animation {
    std::string name;
    float       duration = 0.f;
    std::vector<AnimChannel> channels;
};

// Full model as loaded from GLTF
struct Model {
    Mesh               mesh;
    std::vector<Joint> joints;
    std::vector<Animation> animations;
    bool               hasSkin = false;

    // Compute final skinning matrices for a given animation at time t
    void computeSkinMatrices(int animIndex, float time,
                             std::vector<Mat4>& outJointMatrices) const;
    void destroy() { mesh.destroy(); }
};

class GLTFLoader {
public:
    // Load from file path (for Android asset paths use assetManager variant)
    static bool load(const std::string& path, Model& out);

    // Load from memory (use for Android asset manager reads)
    static bool loadFromMemory(const uint8_t* data, size_t size,
                               const std::string& baseDir, Model& out);
};
