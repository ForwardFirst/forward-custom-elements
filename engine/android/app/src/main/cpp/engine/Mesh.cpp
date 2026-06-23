#include "Mesh.h"

void Mesh::upload(const std::vector<Vertex>& verts,
                  const std::vector<uint32_t>& indices,
                  int matIndex) {
    Primitive prim;
    prim.materialIndex = matIndex;

    glGenVertexArrays(1, &prim.vao);
    glGenBuffers(1, &prim.vbo);
    glBindVertexArray(prim.vao);

    glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    // position (loc 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));
    // normal (loc 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));
    // uv (loc 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));
    // joints (loc 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,joints));
    // weights (loc 4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,weights));

    if(!indices.empty()) {
        glGenBuffers(1, &prim.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
        prim.indexCount  = (int)indices.size();
        prim.hasIndices  = true;
    } else {
        prim.indexCount  = (int)verts.size();
        prim.hasIndices  = false;
    }

    glBindVertexArray(0);
    primitives.push_back(prim);
}

void Mesh::destroy() {
    for(auto& p : primitives) {
        glDeleteVertexArrays(1,&p.vao);
        glDeleteBuffers(1,&p.vbo);
        if(p.ebo) glDeleteBuffers(1,&p.ebo);
    }
    for(auto& mat : materials) {
        if(mat.albedoTex)        glDeleteTextures(1,&mat.albedoTex);
        if(mat.normalTex)        glDeleteTextures(1,&mat.normalTex);
        if(mat.metallicRoughTex) glDeleteTextures(1,&mat.metallicRoughTex);
    }
    primitives.clear();
    materials.clear();
}
