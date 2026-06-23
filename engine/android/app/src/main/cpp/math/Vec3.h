#pragma once
#include <cmath>
#include <algorithm>

struct Vec2 {
    float x = 0.f, y = 0.f;
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s) const { return {x*s, y*s}; }
    float length() const { return std::sqrt(x*x + y*y); }
};

struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s)        const { return {x*s,   y*s,   z*s};   }
    Vec3 operator/(float s)        const { return {x/s,   y/s,   z/s};   }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
    Vec3& operator*=(float s)       { x*=s;   y*=s;   z*=s;   return *this; }
    Vec3 operator-() const { return {-x,-y,-z}; }
    bool operator==(const Vec3& o) const { return x==o.x && y==o.y && z==o.z; }

    float  length()     const { return std::sqrt(x*x + y*y + z*z); }
    float  lengthSq()   const { return x*x + y*y + z*z; }
    Vec3   normalized() const { float l = length(); return l>0.f ? *this/l : Vec3{}; }
    void   normalize()        { float l = length(); if(l>0.f){x/=l;y/=l;z/=l;} }

    float  dot(const Vec3& o)    const { return x*o.x + y*o.y + z*o.z; }
    Vec3   cross(const Vec3& o)  const {
        return { y*o.z - z*o.y,
                 z*o.x - x*o.z,
                 x*o.y - y*o.x };
    }

    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a + (b - a) * t;
    }
    static Vec3 zero()    { return {0,0,0}; }
    static Vec3 up()      { return {0,1,0}; }
    static Vec3 forward() { return {0,0,-1}; }
    static Vec3 right()   { return {1,0,0}; }
};

struct Vec4 {
    float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
    Vec4() = default;
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
    Vec3 xyz() const { return {x,y,z}; }
};
