#include "GLTFLoader.h"
#include <android/log.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

// We use the single-header tiny_gltf (nlohmann/json based)
// but implement a lightweight parser ourselves to avoid heavy deps.
// Supports GLTF 2.0 JSON + embedded base64 binary and external .bin files.

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/tiny_gltf.h"

#define TAG "GLTFLoader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG,__VA_ARGS__)

static GLuint uploadTexture(const tinygltf::Image& img) {
    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    GLenum fmt = img.component==4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D,0,fmt,img.width,img.height,0,fmt,GL_UNSIGNED_BYTE,img.image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D,0);
    return tex;
}

template<typename T>
static const T* bufferPtr(const tinygltf::Model& m, int accIdx) {
    const auto& acc = m.accessors[accIdx];
    const auto& bv  = m.bufferViews[acc.bufferView];
    const auto& buf = m.buffers[bv.buffer];
    return reinterpret_cast<const T*>(buf.data.data() + bv.byteOffset + acc.byteOffset);
}

static bool loadModel(tinygltf::Model& gltf, const uint8_t* data, size_t size,
                      const std::string& baseDir) {
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    bool ok;
    // Detect binary GLB vs JSON
    if(size >= 4 && data[0]=='g' && data[1]=='l' && data[2]=='T' && data[3]=='F') {
        ok = loader.LoadBinaryFromMemory(&gltf, &err, &warn,
                                         data, (unsigned int)size, baseDir);
    } else {
        std::string jsonStr(reinterpret_cast<const char*>(data), size);
        ok = loader.LoadASCIIFromString(&gltf, &err, &warn, jsonStr.c_str(),
                                        (unsigned int)jsonStr.size(), baseDir);
    }
    if(!warn.empty()) LOGE("GLTF warn: %s", warn.c_str());
    if(!err.empty())  LOGE("GLTF err: %s", err.c_str());
    return ok;
}

bool GLTFLoader::loadFromMemory(const uint8_t* data, size_t size,
                                const std::string& baseDir, Model& out) {
    tinygltf::Model gltf;
    if(!loadModel(gltf, data, size, baseDir)) return false;

    // Upload textures
    std::vector<GLuint> textures;
    for(auto& img : gltf.images) textures.push_back(uploadTexture(img));

    // Materials
    for(auto& mat : gltf.materials) {
        Material m;
        auto& pbr = mat.pbrMetallicRoughness;
        m.baseColorFactor = {
            (float)pbr.baseColorFactor[0],
            (float)pbr.baseColorFactor[1],
            (float)pbr.baseColorFactor[2]
        };
        m.metallicFactor  = (float)pbr.metallicFactor;
        m.roughnessFactor = (float)pbr.roughnessFactor;
        if(pbr.baseColorTexture.index >= 0)
            m.albedoTex = textures[gltf.textures[pbr.baseColorTexture.index].source];
        if(mat.normalTexture.index >= 0)
            m.normalTex = textures[gltf.textures[mat.normalTexture.index].source];
        if(pbr.metallicRoughnessTexture.index >= 0)
            m.metallicRoughTex = textures[gltf.textures[pbr.metallicRoughnessTexture.index].source];
        m.doubleSided = mat.doubleSided;
        out.mesh.materials.push_back(m);
    }

    // Meshes
    for(auto& node : gltf.nodes) {
        if(node.mesh < 0) continue;
        auto& gmesh = gltf.meshes[node.mesh];
        for(auto& prim : gmesh.primitives) {
            std::vector<Vertex>   verts;
            std::vector<uint32_t> indices;

            // Positions
            const float* pos = nullptr;
            const float* nrm = nullptr;
            const float* uv0 = nullptr;
            const uint8_t*  joints8  = nullptr;
            const uint16_t* joints16 = nullptr;
            const float*    weights  = nullptr;
            int posCount = 0;

            for(auto& [attr, acc] : prim.attributes) {
                const auto& accessor = gltf.accessors[acc];
                if(attr=="POSITION") { pos=bufferPtr<float>(gltf,acc); posCount=(int)accessor.count; }
                if(attr=="NORMAL")   { nrm=bufferPtr<float>(gltf,acc); }
                if(attr=="TEXCOORD_0"){ uv0=bufferPtr<float>(gltf,acc); }
                if(attr=="JOINTS_0") {
                    if(accessor.componentType==TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                        joints8 = bufferPtr<uint8_t>(gltf,acc);
                    else
                        joints16= bufferPtr<uint16_t>(gltf,acc);
                }
                if(attr=="WEIGHTS_0"){ weights=bufferPtr<float>(gltf,acc); }
            }

            verts.resize(posCount);
            for(int i=0;i<posCount;i++) {
                verts[i].position = {pos[i*3],pos[i*3+1],pos[i*3+2]};
                if(nrm) verts[i].normal = {nrm[i*3],nrm[i*3+1],nrm[i*3+2]};
                if(uv0) { verts[i].uv[0]=uv0[i*2]; verts[i].uv[1]=uv0[i*2+1]; }
                for(int j=0;j<4;j++){
                    if(joints8)  verts[i].joints[j]  = (float)joints8 [i*4+j];
                    if(joints16) verts[i].joints[j]  = (float)joints16[i*4+j];
                    if(weights)  verts[i].weights[j] = weights[i*4+j];
                }
            }

            if(prim.indices >= 0) {
                const auto& iacc = gltf.accessors[prim.indices];
                indices.resize(iacc.count);
                if(iacc.componentType==TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    auto* src = bufferPtr<uint32_t>(gltf, prim.indices);
                    for(size_t i=0;i<iacc.count;i++) indices[i]=src[i];
                } else if(iacc.componentType==TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    auto* src = bufferPtr<uint16_t>(gltf, prim.indices);
                    for(size_t i=0;i<iacc.count;i++) indices[i]=src[i];
                } else {
                    auto* src = bufferPtr<uint8_t>(gltf, prim.indices);
                    for(size_t i=0;i<iacc.count;i++) indices[i]=src[i];
                }
            }

            out.mesh.upload(verts, indices, prim.material);
        }
    }

    // Skin / skeleton
    if(!gltf.skins.empty()) {
        out.hasSkin = true;
        auto& skin  = gltf.skins[0];
        out.joints.resize(skin.joints.size());

        // Inverse bind matrices
        const float* ibm = nullptr;
        if(skin.inverseBindMatrices >= 0)
            ibm = bufferPtr<float>(gltf, skin.inverseBindMatrices);

        for(int i=0;i<(int)skin.joints.size();i++) {
            int nodeIdx = skin.joints[i];
            auto& node  = gltf.nodes[nodeIdx];
            out.joints[i].name = node.name;

            if(ibm) memcpy(out.joints[i].inverseBindMatrix.m, ibm+i*16, 64);

            if(!node.translation.empty())
                out.joints[i].localTranslation = {(float)node.translation[0],(float)node.translation[1],(float)node.translation[2]};
            if(!node.rotation.empty())
                out.joints[i].localRotation = {(float)node.rotation[0],(float)node.rotation[1],(float)node.rotation[2],(float)node.rotation[3]};
            if(!node.scale.empty())
                out.joints[i].localScale = {(float)node.scale[0],(float)node.scale[1],(float)node.scale[2]};

            // Find parent
            for(int p=0;p<(int)skin.joints.size();p++) {
                int pNodeIdx = skin.joints[p];
                auto& pNode  = gltf.nodes[pNodeIdx];
                for(int ch : pNode.children)
                    if(ch == nodeIdx) out.joints[i].parent = p;
            }
        }

        // Animations
        for(auto& ganim : gltf.animations) {
            Animation anim;
            anim.name = ganim.name;

            for(auto& chan : ganim.channels) {
                AnimChannel ac;
                // Map GLTF node index → joint index
                auto it = std::find(skin.joints.begin(), skin.joints.end(), chan.target_node);
                if(it == skin.joints.end()) continue;
                ac.jointIndex = (int)(it - skin.joints.begin());

                if(chan.target_path=="translation") ac.path = AnimChannel::Path::Translation;
                else if(chan.target_path=="rotation") ac.path = AnimChannel::Path::Rotation;
                else if(chan.target_path=="scale")    ac.path = AnimChannel::Path::Scale;
                else continue;

                auto& samp  = ganim.samplers[chan.sampler];
                // Times
                const float* times = bufferPtr<float>(gltf, samp.input);
                int nKeys = (int)gltf.accessors[samp.input].count;
                ac.times.assign(times, times+nKeys);
                if(!ac.times.empty()) anim.duration = std::max(anim.duration, ac.times.back());

                // Values
                const float* vals = bufferPtr<float>(gltf, samp.output);
                int valComp = (ac.path==AnimChannel::Path::Rotation) ? 4 : 3;
                ac.values.assign(vals, vals + nKeys*valComp);

                if(samp.interpolation=="STEP")        ac.interp = AnimChannel::Interp::Step;
                else if(samp.interpolation=="CUBICSPLINE") ac.interp = AnimChannel::Interp::CubicSpline;

                anim.channels.push_back(std::move(ac));
            }
            out.animations.push_back(std::move(anim));
        }
    }

    LOGI("Loaded model: %d primitives, %d joints, %d animations",
         (int)out.mesh.primitives.size(), (int)out.joints.size(), (int)out.animations.size());
    return true;
}

bool GLTFLoader::load(const std::string& path, Model& out) {
    std::ifstream f(path, std::ios::binary);
    if(!f) { LOGE("Cannot open %s", path.c_str()); return false; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), {});
    std::string baseDir = path.substr(0, path.find_last_of("/\\")+1);
    return loadFromMemory(data.data(), data.size(), baseDir, out);
}

// ---- Animation evaluation ----
void Model::computeSkinMatrices(int animIndex, float time,
                                std::vector<Mat4>& out) const {
    int n = (int)joints.size();
    out.resize(n);
    if(n == 0) return;

    std::vector<Vec3>       T(n), S(n);
    std::vector<Quaternion> R(n);
    for(int i=0;i<n;i++){
        T[i]=joints[i].localTranslation;
        R[i]=joints[i].localRotation;
        S[i]=joints[i].localScale;
    }

    if(animIndex >= 0 && animIndex < (int)animations.size()) {
        const auto& anim = animations[animIndex];
        float t = std::fmod(time, anim.duration > 0 ? anim.duration : 1.f);

        for(auto& ch : anim.channels) {
            int ji = ch.jointIndex;
            if(ji >= n) continue;
            int nKeys = (int)ch.times.size();
            if(nKeys == 0) continue;

            // Find surrounding keys
            int k1 = 0;
            for(int k=0;k<nKeys-1;k++) if(ch.times[k] <= t) k1=k;
            int k2 = std::min(k1+1, nKeys-1);
            float alpha = 0.f;
            if(ch.times[k2] > ch.times[k1])
                alpha = (t-ch.times[k1])/(ch.times[k2]-ch.times[k1]);

            if(ch.path == AnimChannel::Path::Translation) {
                Vec3 a={ch.values[k1*3],ch.values[k1*3+1],ch.values[k1*3+2]};
                Vec3 b={ch.values[k2*3],ch.values[k2*3+1],ch.values[k2*3+2]};
                T[ji] = ch.interp==AnimChannel::Interp::Step ? a : Vec3::lerp(a,b,alpha);
            } else if(ch.path == AnimChannel::Path::Rotation) {
                Quaternion a={ch.values[k1*4],ch.values[k1*4+1],ch.values[k1*4+2],ch.values[k1*4+3]};
                Quaternion b={ch.values[k2*4],ch.values[k2*4+1],ch.values[k2*4+2],ch.values[k2*4+3]};
                R[ji] = ch.interp==AnimChannel::Interp::Step ? a : Quaternion::slerp(a,b,alpha);
            } else {
                Vec3 a={ch.values[k1*3],ch.values[k1*3+1],ch.values[k1*3+2]};
                Vec3 b={ch.values[k2*3],ch.values[k2*3+1],ch.values[k2*3+2]};
                S[ji] = ch.interp==AnimChannel::Interp::Step ? a : Vec3::lerp(a,b,alpha);
            }
        }
    }

    // Build global transforms
    std::vector<Mat4> global(n);
    for(int i=0;i<n;i++){
        Mat4 local = Mat4::Translation(T[i])
                   * R[i].toMatrix()
                   * Mat4::Scale(S[i].x,S[i].y,S[i].z);
        if(joints[i].parent < 0)
            global[i] = local;
        else
            global[i] = global[joints[i].parent] * local;
        out[i] = global[i] * joints[i].inverseBindMatrix;
    }
}
