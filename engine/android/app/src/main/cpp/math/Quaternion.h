#pragma once
#include "Vec3.h"
#include "Mat4.h"
#include <cmath>

struct Quaternion {
    float x=0,y=0,z=0,w=1;
    Quaternion() = default;
    Quaternion(float x,float y,float z,float w):x(x),y(y),z(z),w(w){}

    Quaternion operator*(const Quaternion& b) const {
        return {
            w*b.x + x*b.w + y*b.z - z*b.y,
            w*b.y - x*b.z + y*b.w + z*b.x,
            w*b.z + x*b.y - y*b.x + z*b.w,
            w*b.w - x*b.x - y*b.y - z*b.z
        };
    }
    Quaternion& operator*=(const Quaternion& b) { *this = *this * b; return *this; }

    float length()  const { return std::sqrt(x*x+y*y+z*z+w*w); }
    Quaternion normalized() const { float l=length(); return {x/l,y/l,z/l,w/l}; }
    Quaternion conjugate()  const { return {-x,-y,-z,w}; }

    Vec3 rotate(const Vec3& v) const {
        Quaternion p(v.x,v.y,v.z,0);
        Quaternion r = *this * p * conjugate();
        return {r.x,r.y,r.z};
    }

    Mat4 toMatrix() const { return Mat4::FromQuaternion(x,y,z,w); }

    static Quaternion identity() { return {0,0,0,1}; }
    static Quaternion fromAxisAngle(const Vec3& axis, float rad) {
        float s = std::sin(rad*0.5f);
        Vec3 a = axis.normalized();
        return {a.x*s, a.y*s, a.z*s, std::cos(rad*0.5f)};
    }
    static Quaternion fromEuler(float pitch, float yaw, float roll) {
        Quaternion qx = fromAxisAngle({1,0,0}, pitch);
        Quaternion qy = fromAxisAngle({0,1,0}, yaw);
        Quaternion qz = fromAxisAngle({0,0,1}, roll);
        return (qy * qx * qz).normalized();
    }
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
        float dot = a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
        Quaternion bb = dot < 0 ? Quaternion{-b.x,-b.y,-b.z,-b.w} : b;
        dot = std::abs(dot);
        if(dot > 0.9995f) {
            return Quaternion{
                a.x+(bb.x-a.x)*t, a.y+(bb.y-a.y)*t,
                a.z+(bb.z-a.z)*t, a.w+(bb.w-a.w)*t
            }.normalized();
        }
        float theta0 = std::acos(dot);
        float theta  = theta0 * t;
        float s0 = std::cos(theta) - dot*std::sin(theta)/std::sin(theta0);
        float s1 = std::sin(theta)/std::sin(theta0);
        return Quaternion{
            s0*a.x+s1*bb.x, s0*a.y+s1*bb.y,
            s0*a.z+s1*bb.z, s0*a.w+s1*bb.w
        };
    }
};
