#pragma once
#include "Vec3.h"
#include <cmath>
#include <cstring>

// Column-major 4x4 matrix (matches OpenGL convention)
struct Mat4 {
    float m[16] = {};

    Mat4() { identity(); }

    void identity() {
        memset(m, 0, sizeof(m));
        m[0]=m[5]=m[10]=m[15]=1.f;
    }

    static Mat4 Identity() { return Mat4{}; }

    // Access: col*4 + row
    float& operator()(int row, int col) { return m[col*4+row]; }
    float  operator()(int row, int col) const { return m[col*4+row]; }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        for(int c=0;c<4;c++)
            for(int r2=0;r2<4;r2++){
                float sum=0;
                for(int k=0;k<4;k++) sum += (*this)(r2,k)*b(k,c);
                r(r2,c) = sum;
            }
        return r;
    }

    Vec4 operator*(const Vec4& v) const {
        return {
            m[0]*v.x + m[4]*v.y + m[8]*v.z  + m[12]*v.w,
            m[1]*v.x + m[5]*v.y + m[9]*v.z  + m[13]*v.w,
            m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,
            m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w
        };
    }

    Vec3 transformPoint(const Vec3& p) const {
        Vec4 v = *this * Vec4(p, 1.f);
        return {v.x, v.y, v.z};
    }
    Vec3 transformDir(const Vec3& d) const {
        Vec4 v = *this * Vec4(d, 0.f);
        return {v.x, v.y, v.z};
    }

    Mat4 transposed() const {
        Mat4 r;
        for(int i=0;i<4;i++)
            for(int j=0;j<4;j++)
                r(i,j) = (*this)(j,i);
        return r;
    }

    // --- static constructors ---
    static Mat4 Translation(float tx, float ty, float tz) {
        Mat4 r;
        r(0,3)=tx; r(1,3)=ty; r(2,3)=tz;
        return r;
    }
    static Mat4 Translation(const Vec3& t) { return Translation(t.x,t.y,t.z); }

    static Mat4 Scale(float sx, float sy, float sz) {
        Mat4 r;
        r(0,0)=sx; r(1,1)=sy; r(2,2)=sz;
        return r;
    }
    static Mat4 Scale(float s) { return Scale(s,s,s); }

    static Mat4 RotationX(float rad) {
        Mat4 r;
        r(1,1)= std::cos(rad); r(1,2)=-std::sin(rad);
        r(2,1)= std::sin(rad); r(2,2)= std::cos(rad);
        return r;
    }
    static Mat4 RotationY(float rad) {
        Mat4 r;
        r(0,0)= std::cos(rad); r(0,2)= std::sin(rad);
        r(2,0)=-std::sin(rad); r(2,2)= std::cos(rad);
        return r;
    }
    static Mat4 RotationZ(float rad) {
        Mat4 r;
        r(0,0)= std::cos(rad); r(0,1)=-std::sin(rad);
        r(1,0)= std::sin(rad); r(1,1)= std::cos(rad);
        return r;
    }
    static Mat4 AxisAngle(const Vec3& axis, float rad) {
        Vec3 a = axis.normalized();
        float c = std::cos(rad), s = std::sin(rad), t = 1.f-c;
        Mat4 r;
        r(0,0)=t*a.x*a.x+c;      r(0,1)=t*a.x*a.y-s*a.z; r(0,2)=t*a.x*a.z+s*a.y;
        r(1,0)=t*a.x*a.y+s*a.z;  r(1,1)=t*a.y*a.y+c;      r(1,2)=t*a.y*a.z-s*a.x;
        r(2,0)=t*a.x*a.z-s*a.y;  r(2,1)=t*a.y*a.z+s*a.x;  r(2,2)=t*a.z*a.z+c;
        return r;
    }
    static Mat4 FromQuaternion(float qx, float qy, float qz, float qw) {
        Mat4 r;
        float x2=qx*qx,y2=qy*qy,z2=qz*qz;
        r(0,0)=1-2*(y2+z2); r(0,1)=2*(qx*qy-qz*qw); r(0,2)=2*(qx*qz+qy*qw);
        r(1,0)=2*(qx*qy+qz*qw); r(1,1)=1-2*(x2+z2); r(1,2)=2*(qy*qz-qx*qw);
        r(2,0)=2*(qx*qz-qy*qw); r(2,1)=2*(qy*qz+qx*qw); r(2,2)=1-2*(x2+y2);
        return r;
    }

    static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 r = f.cross(up).normalized();
        Vec3 u = r.cross(f);
        Mat4 m;
        m(0,0)= r.x; m(0,1)= r.y; m(0,2)= r.z; m(0,3)=-r.dot(eye);
        m(1,0)= u.x; m(1,1)= u.y; m(1,2)= u.z; m(1,3)=-u.dot(eye);
        m(2,0)=-f.x; m(2,1)=-f.y; m(2,2)=-f.z; m(2,3)= f.dot(eye);
        m(3,3)=1.f;
        return m;
    }

    static Mat4 Perspective(float fovY, float aspect, float zNear, float zFar) {
        float tanH = std::tan(fovY * 0.5f);
        Mat4 r;
        r.m[0]  = 1.f/(aspect*tanH);
        r.m[5]  = 1.f/tanH;
        r.m[10] = -(zFar+zNear)/(zFar-zNear);
        r.m[11] = -1.f;
        r.m[14] = -(2.f*zFar*zNear)/(zFar-zNear);
        r.m[15] = 0.f;
        return r;
    }

    static Mat4 Ortho(float l,float r2,float b,float t,float n,float f) {
        Mat4 m;
        m(0,0)=2/(r2-l); m(0,3)=-(r2+l)/(r2-l);
        m(1,1)=2/(t-b);  m(1,3)=-(t+b)/(t-b);
        m(2,2)=-2/(f-n); m(2,3)=-(f+n)/(f-n);
        return m;
    }

    // Inverse for affine transforms (assumes bottom row = [0,0,0,1])
    Mat4 affineInverse() const {
        Mat4 inv;
        // Transpose upper 3x3
        for(int i=0;i<3;i++)
            for(int j=0;j<3;j++)
                inv(i,j)=(*this)(j,i);
        // -R^T * t
        Vec3 t = {(*this)(0,3),(*this)(1,3),(*this)(2,3)};
        Vec3 ti = {
            -(inv(0,0)*t.x+inv(0,1)*t.y+inv(0,2)*t.z),
            -(inv(1,0)*t.x+inv(1,1)*t.y+inv(1,2)*t.z),
            -(inv(2,0)*t.x+inv(2,1)*t.y+inv(2,2)*t.z)
        };
        inv(0,3)=ti.x; inv(1,3)=ti.y; inv(2,3)=ti.z;
        return inv;
    }
};
