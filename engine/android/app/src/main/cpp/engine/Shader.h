#pragma once
#include <GLES3/gl3.h>
#include <string>
#include <unordered_map>
#include "../math/Mat4.h"
#include "../math/Vec3.h"

class Shader {
public:
    GLuint program = 0;

    bool load(const char* vertSrc, const char* fragSrc);
    void use() const;
    void destroy();

    void setInt   (const char* name, int v)          const;
    void setFloat (const char* name, float v)        const;
    void setVec2  (const char* name, float x,float y) const;
    void setVec3  (const char* name, const Vec3& v)  const;
    void setVec4  (const char* name, float x,float y,float z,float w) const;
    void setMat4  (const char* name, const Mat4& m)  const;
    void setMat4Array(const char* name, const Mat4* mats, int count) const;

private:
    mutable std::unordered_map<std::string, GLint> uniformCache;
    GLint loc(const char* name) const;
    static GLuint compile(GLenum type, const char* src);
};
