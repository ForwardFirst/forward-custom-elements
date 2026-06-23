#include "Shader.h"
#include <android/log.h>
#include <vector>

#define LOG_TAG "Shader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

GLuint Shader::compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok) {
        GLint len;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        LOGE("Shader compile error: %s", log.data());
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::load(const char* vertSrc, const char* fragSrc) {
    GLuint vert = compile(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compile(GL_FRAGMENT_SHADER, fragSrc);
    if(!vert || !frag) return false;

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if(!ok) {
        GLint len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(program, len, nullptr, log.data());
        LOGE("Program link error: %s", log.data());
        glDeleteProgram(program);
        program = 0;
        return false;
    }
    return true;
}

void Shader::use() const { glUseProgram(program); }
void Shader::destroy()   { if(program){glDeleteProgram(program);program=0;} }

GLint Shader::loc(const char* name) const {
    auto it = uniformCache.find(name);
    if(it != uniformCache.end()) return it->second;
    GLint l = glGetUniformLocation(program, name);
    uniformCache[name] = l;
    return l;
}

void Shader::setInt  (const char* n, int v)           const { glUniform1i(loc(n),v); }
void Shader::setFloat(const char* n, float v)         const { glUniform1f(loc(n),v); }
void Shader::setVec2 (const char* n, float x,float y) const { glUniform2f(loc(n),x,y); }
void Shader::setVec3 (const char* n, const Vec3& v)   const { glUniform3f(loc(n),v.x,v.y,v.z); }
void Shader::setVec4 (const char* n, float x,float y,float z,float w) const { glUniform4f(loc(n),x,y,z,w); }
void Shader::setMat4 (const char* n, const Mat4& m)   const { glUniformMatrix4fv(loc(n),1,GL_FALSE,m.m); }
void Shader::setMat4Array(const char* n, const Mat4* mats, int count) const {
    glUniformMatrix4fv(loc(n), count, GL_FALSE, mats[0].m);
}
