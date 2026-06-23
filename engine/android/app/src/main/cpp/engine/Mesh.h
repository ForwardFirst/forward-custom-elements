#pragma once
#include <GLES3/gl3.h>
#include <vector>
#include "../math/Vec3.h"
#include "../math/Mat4.h"

// Max bones per vertex for skeletal animation
static constexpr int MAX_JOINTS = 4;
// Max bones in a skeleton
static constexpr int MAX_SKELETON_JOINTS = 64;

struct Vertex {
    Vec3  position;
    Vec3  normal;
    float uv[2]     = {};
    float joints[4] = {};   // bone indices
    float weights[4]= {};   // blend weights
};

struct Primitive {
    GLuint vao = 0, vbo = 0, ebo = 0;
    int    indexCount = 0;
    int    materialIndex = -1;
    bool   hasIndices = false;
};

struct Material {
    Vec3  baseColorFactor  = {1,1,1};
    float metallicFactor   = 0.0f;
    float roughnessFactor  = 0.5f;
    GLuint albedoTex       = 0;
    GLuint normalTex       = 0;
    GLuint metallicRoughTex= 0;
    bool   doubleSided     = false;
};

struct Mesh {
    std::vector<Primitive> primitives;
    std::vector<Material>  materials;

    void upload(const std::vector<Vertex>& verts,
                const std::vector<uint32_t>& indices,
                int matIndex = 0);
    void destroy();
};
