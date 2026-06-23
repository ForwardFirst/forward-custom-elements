#include "Renderer.h"
#include <android/log.h>
#include <algorithm>

#define TAG  "Renderer"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG,__VA_ARGS__)

// ---- Shader sources ----

static const char* PBR_VERT = R"GLSL(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aJoints;
layout(location=4) in vec4 aWeights;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uJoints[64];
uniform int  uSkinned;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main(){
    vec4 localPos  = vec4(aPos,1.0);
    vec3 localNorm = aNormal;

    if(uSkinned==1){
        mat4 skin = aWeights.x*uJoints[int(aJoints.x)]
                  + aWeights.y*uJoints[int(aJoints.y)]
                  + aWeights.z*uJoints[int(aJoints.z)]
                  + aWeights.w*uJoints[int(aJoints.w)];
        localPos  = skin*localPos;
        localNorm = mat3(skin)*aNormal;
    }

    vec4 worldPos4 = uModel*localPos;
    vWorldPos = worldPos4.xyz;
    vNormal   = normalize(mat3(uModel)*localNorm);
    vUV       = aUV;
    gl_Position = uProj*uView*worldPos4;
}
)GLSL";

static const char* PBR_FRAG = R"GLSL(#version 300 es
precision mediump float;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform vec3  uBaseColor;
uniform float uRoughness;
uniform float uMetallic;
uniform sampler2D uAlbedoTex;
uniform int  uHasAlbedo;
uniform vec3 uCamPos;
uniform vec3 uSunDir;
uniform vec3 uSunColor;

// Up to 4 point lights
uniform vec3  uLightPos[4];
uniform vec3  uLightColor[4];
uniform float uLightRadius[4];
uniform int   uNumLights;

out vec4 fragColor;

const float PI = 3.14159265;

float D_GGX(float NdH, float r){ float a=r*r; float b=NdH*NdH*(a*a-1.0)+1.0; return a*a/(PI*b*b); }
float G_Smith(float NdV, float NdL, float r){ float k=(r+1.0)*(r+1.0)/8.0; return (NdV/(NdV*(1.0-k)+k))*(NdL/(NdL*(1.0-k)+k)); }
vec3 F_Schlick(float cos, vec3 F0){ return F0+(1.0-F0)*pow(1.0-cos,5.0); }

vec3 pbrLight(vec3 N, vec3 V, vec3 L, vec3 lColor, vec3 albedo, float metal, float rough){
    vec3  F0    = mix(vec3(0.04), albedo, metal);
    vec3  H     = normalize(V+L);
    float NdL   = max(dot(N,L),0.0);
    float NdV   = max(dot(N,V),0.001);
    float NdH   = max(dot(N,H),0.0);
    float HdV   = max(dot(H,V),0.0);
    float D     = D_GGX(NdH, rough);
    float G     = G_Smith(NdV, NdL, rough);
    vec3  F     = F_Schlick(HdV, F0);
    vec3  spec  = D*G*F/(4.0*NdV*NdL+0.001);
    vec3  diff  = (1.0-F)*(1.0-metal)*albedo/PI;
    return (diff+spec)*lColor*NdL;
}

void main(){
    vec3 albedo = uBaseColor;
    if(uHasAlbedo==1) albedo *= texture(uAlbedoTex, vUV).rgb;
    float metal  = uMetallic;
    float rough  = max(uRoughness, 0.05);
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);

    vec3 Lo = vec3(0.0);
    // Sun (directional)
    Lo += pbrLight(N, V, normalize(-uSunDir), uSunColor, albedo, metal, rough);

    // Point lights
    for(int i=0;i<uNumLights && i<4;i++){
        vec3 LV  = uLightPos[i]-vWorldPos;
        float d  = length(LV);
        float att= clamp(1.0-d/uLightRadius[i],0.0,1.0);
        att      *= att;
        Lo += pbrLight(N, V, normalize(LV), uLightColor[i]*att, albedo, metal, rough);
    }

    vec3 ambient = albedo*0.08;
    vec3 color   = ambient + Lo;
    // Tone-map (Reinhard)
    color = color/(color+vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    fragColor = vec4(color, 1.0);
}
)GLSL";

// ---- Renderer impl ----

bool Renderer::init(int w, int h) {
    width=w; height=h;

    if(!pbrShader.load(PBR_VERT, PBR_FRAG)) {
        LOGE("Failed to compile PBR shader"); return false;
    }
    skinShader = pbrShader;  // same shader, uSkinned=1 path

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    LOGI("Renderer initialised %dx%d", w, h);
    return true;
}

void Renderer::resize(int w, int h) {
    width=w; height=h;
    glViewport(0,0,w,h);
}

void Renderer::destroy() {
    pbrShader.destroy();
    skinShader.destroy();
}

void Renderer::setLights(const std::vector<PointLight>& lights) { pointLights=lights; }
void Renderer::setSunDirection(const Vec3& dir, const Vec3& color){ sunDir=dir; sunColor=color; }

void Renderer::beginFrame() {
    glViewport(0,0,width,height);
    glClearColor(0.4f,0.65f,0.9f,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {}

void Renderer::bindMaterialUniforms(const Shader& sh, const Material& mat) {
    sh.setVec3("uBaseColor", mat.baseColorFactor);
    sh.setFloat("uRoughness", mat.roughnessFactor);
    sh.setFloat("uMetallic",  mat.metallicFactor);
    if(mat.albedoTex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mat.albedoTex);
        sh.setInt("uAlbedoTex", 0);
        sh.setInt("uHasAlbedo", 1);
    } else {
        sh.setInt("uHasAlbedo", 0);
    }
}

void Renderer::drawPrimitive(const Shader& sh, const Primitive& prim) {
    glBindVertexArray(prim.vao);
    if(prim.hasIndices)
        glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, nullptr);
    else
        glDrawArrays(GL_TRIANGLES, 0, prim.indexCount);
    glBindVertexArray(0);
}

static void setCommonUniforms(const Shader& sh, const Mat4& model,
                              const Camera& cam,
                              const Vec3& sunDir, const Vec3& sunColor,
                              const std::vector<PointLight>& lights) {
    sh.setMat4("uModel", model);
    sh.setMat4("uView",  cam.view());
    sh.setMat4("uProj",  cam.projection());
    sh.setVec3("uCamPos",  cam.position());
    sh.setVec3("uSunDir",  sunDir);
    sh.setVec3("uSunColor",sunColor);

    int n = (int)std::min((int)lights.size(), 4);
    sh.setInt("uNumLights", n);
    for(int i=0;i<n;i++){
        char buf[64];
        snprintf(buf,64,"uLightPos[%d]",i);   sh.setVec3(buf,  lights[i].position);
        snprintf(buf,64,"uLightColor[%d]",i); sh.setVec3(buf,  lights[i].color * lights[i].intensity);
        snprintf(buf,64,"uLightRadius[%d]",i);sh.setFloat(buf, lights[i].radius);
    }
}

void Renderer::drawTerrain(const Terrain& terrain, const Camera& cam) {
    pbrShader.use();
    setCommonUniforms(pbrShader, Mat4::Identity(), cam, sunDir, sunColor, pointLights);
    pbrShader.setInt("uSkinned", 0);
    for(auto& prim : terrain.mesh.primitives) {
        int mi = prim.materialIndex >= 0 ? prim.materialIndex : 0;
        if(mi < (int)terrain.mesh.materials.size())
            bindMaterialUniforms(pbrShader, terrain.mesh.materials[mi]);
        drawPrimitive(pbrShader, prim);
    }
}

void Renderer::drawMesh(const DrawCall& dc, const Camera& cam) {
    const Shader& sh = pbrShader;
    sh.use();
    setCommonUniforms(sh, dc.modelMatrix, cam, sunDir, sunColor, pointLights);

    bool skinned = !dc.jointMatrices.empty();
    sh.setInt("uSkinned", skinned ? 1 : 0);
    if(skinned)
        sh.setMat4Array("uJoints", dc.jointMatrices.data(),
                        (int)std::min((int)dc.jointMatrices.size(), MAX_SKELETON_JOINTS));

    for(auto& prim : dc.mesh->primitives) {
        int mi = prim.materialIndex;
        if(mi >= 0 && mi < (int)dc.mesh->materials.size())
            bindMaterialUniforms(sh, dc.mesh->materials[mi]);
        drawPrimitive(sh, prim);
    }
}

void Renderer::drawMeshes(const std::vector<DrawCall>& dcs, const Camera& cam) {
    for(auto& dc : dcs) drawMesh(dc, cam);
}
