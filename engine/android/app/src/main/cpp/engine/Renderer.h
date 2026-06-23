#pragma once
#include <GLES3/gl3.h>
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "Terrain.h"
#include "GLTFLoader.h"
#include "../math/Mat4.h"
#include <vector>

struct DrawCall {
    const Mesh* mesh          = nullptr;
    Mat4        modelMatrix;
    int         animIndex     = -1;
    float       animTime      = 0.f;
    std::vector<Mat4> jointMatrices;  // filled by renderer
};

struct PointLight {
    Vec3  position;
    Vec3  color;
    float intensity;
    float radius;
};

class Renderer {
public:
    bool init(int w, int h);
    void resize(int w, int h);
    void destroy();

    void beginFrame();
    void drawTerrain(const Terrain& terrain, const Camera& cam);
    void drawMesh(const DrawCall& dc, const Camera& cam);
    void drawMeshes(const std::vector<DrawCall>& dcs, const Camera& cam);
    void endFrame();

    void setLights(const std::vector<PointLight>& lights);
    void setSunDirection(const Vec3& dir, const Vec3& color);

    int width = 0, height = 0;

private:
    Shader pbrShader;
    Shader terrainShader;
    Shader skinShader;

    Vec3 sunDir   = {0.5f,-1.f,-0.5f};
    Vec3 sunColor = {1.f,0.95f,0.8f};

    std::vector<PointLight> pointLights;

    void bindMaterialUniforms(const Shader& sh, const Material& mat);
    void drawPrimitive(const Shader& sh, const Primitive& prim);
};
