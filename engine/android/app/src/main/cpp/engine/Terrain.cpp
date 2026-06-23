#include "Terrain.h"
#include <cstring>
#include <algorithm>

// Permutation table for smooth noise
static int P[512];
static bool pInit = false;
static void initPerm() {
    if(pInit) return;
    pInit = true;
    int src[256];
    for(int i=0;i<256;i++) src[i]=i;
    // deterministic shuffle
    for(int i=255;i>0;i--){
        int j = (i*9301+49297)%i;
        std::swap(src[i],src[j]);
    }
    for(int i=0;i<512;i++) P[i]=src[i&255];
}

static float fade(float t){ return t*t*t*(t*(t*6-15)+10); }
static float lerp(float a,float b,float t){ return a+t*(b-a); }
static float grad(int h,float x,float z){
    switch(h&3){
        case 0: return  x+z; case 1: return -x+z;
        case 2: return  x-z; default: return -x-z;
    }
}

float Terrain::noise(float x, float z) const {
    initPerm();
    int xi=(int)std::floor(x)&255, zi=(int)std::floor(z)&255;
    float xf=x-std::floor(x), zf=z-std::floor(z);
    float u=fade(xf), v=fade(zf);
    int aa=P[P[xi  ]+zi], ab=P[P[xi  ]+zi+1];
    int ba=P[P[xi+1]+zi], bb=P[P[xi+1]+zi+1];
    return lerp(lerp(grad(aa,xf,zf),  grad(ba,xf-1,zf),  u),
                lerp(grad(ab,xf,zf-1),grad(bb,xf-1,zf-1),u), v);
}

float Terrain::fbm(float x, float z, int octaves) const {
    float val=0, amp=1, freq=1, max=0;
    for(int i=0;i<octaves;i++){
        val += noise(x*freq, z*freq)*amp;
        max += amp;
        amp  *= 0.5f;
        freq *= 2.f;
    }
    return val/max;
}

void Terrain::generate() {
    heights.resize((gridW+1)*(gridH+1));
    float invW = 1.f/gridW, invH = 1.f/gridH;

    for(int z=0;z<=gridH;z++)
        for(int x=0;x<=gridW;x++)
            heights[z*(gridW+1)+x] = fbm(x*invW*4.f, z*invH*4.f, 6) * heightScale;

    std::vector<Vertex>   verts;
    std::vector<uint32_t> indices;

    verts.reserve((gridW+1)*(gridH+1));
    for(int z=0;z<=gridH;z++) {
        for(int x=0;x<=gridW;x++) {
            float h = heights[z*(gridW+1)+x];
            Vertex v;
            v.position = {x*cellSize, h, z*cellSize};
            // Compute normal via central differences
            float hL = (x>0)     ? heights[z*(gridW+1)+x-1] : h;
            float hR = (x<gridW) ? heights[z*(gridW+1)+x+1] : h;
            float hD = (z>0)     ? heights[(z-1)*(gridW+1)+x] : h;
            float hU = (z<gridH) ? heights[(z+1)*(gridW+1)+x] : h;
            v.normal  = Vec3{hL-hR, 2.f*cellSize, hD-hU}.normalized();
            v.uv[0]   = (float)x/gridW*8.f;
            v.uv[1]   = (float)z/gridH*8.f;
            verts.push_back(v);
        }
    }

    indices.reserve(gridW*gridH*6);
    for(int z=0;z<gridH;z++)
        for(int x=0;x<gridW;x++){
            uint32_t tl=z*(gridW+1)+x, tr=tl+1, bl=tl+(gridW+1), br=bl+1;
            indices.insert(indices.end(),{tl,bl,tr, tr,bl,br});
        }

    Material mat;
    mat.baseColorFactor = {0.3f,0.55f,0.2f};
    mat.roughnessFactor = 0.9f;
    mesh.materials.push_back(mat);
    mesh.upload(verts, indices, 0);
}

float Terrain::heightAt(float wx, float wz) const {
    if(heights.empty()) return 0.f;
    float gx = wx/cellSize, gz = wz/cellSize;
    int   x0 = (int)std::floor(gx), z0 = (int)std::floor(gz);
    x0 = std::clamp(x0, 0, gridW-1);
    z0 = std::clamp(z0, 0, gridH-1);
    int x1=x0+1, z1=z0+1;
    x1 = std::min(x1, gridW); z1 = std::min(z1, gridH);
    float tx = gx-x0, tz = gz-z0;
    float h00=heights[z0*(gridW+1)+x0], h10=heights[z0*(gridW+1)+x1];
    float h01=heights[z1*(gridW+1)+x0], h11=heights[z1*(gridW+1)+x1];
    return lerp(lerp(h00,h10,tx), lerp(h01,h11,tx), tz);
}
