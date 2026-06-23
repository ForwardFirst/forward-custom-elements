#pragma once
#include "Mesh.h"
#include "../math/Vec3.h"
#include <vector>
#include <cmath>

// Procedural heightmap terrain using layered noise
class Terrain {
public:
    int   gridW   = 128;
    int   gridH   = 128;
    float cellSize= 1.0f;
    float heightScale = 8.0f;

    Mesh mesh;

    void generate();
    void destroy() { mesh.destroy(); }

    // Sample height at world XZ (linear interp)
    float heightAt(float wx, float wz) const;

    // AABB collision: return ground Y at (wx,wz)
    float groundY(float wx, float wz) const { return heightAt(wx,wz); }

private:
    std::vector<float> heights;

    float noise(float x, float z) const;
    float fbm(float x, float z, int octaves) const;
};
