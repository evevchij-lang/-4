#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <array>
#include <algorithm>

// из main.cpp
extern GLuint CreateShaderProgram(const char* vsPath, const char* fsPath);
extern GLuint LoadTexture2D(const char* path);
extern int g_currentTool;

// доступ к камере (у тебя struct Camera в main.cpp)
struct Camera;
extern Camera g_cam;

// ===================== utils =====================

static glm::mat4 CST_AiToGlm(const aiMatrix4x4& m)
{
    glm::mat4 r;
    r[0][0] = m.a1; r[1][0] = m.a2; r[2][0] = m.a3; r[3][0] = m.a4;
    r[0][1] = m.b1; r[1][1] = m.b2; r[2][1] = m.b3; r[3][1] = m.b4;
    r[0][2] = m.c1; r[1][2] = m.c2; r[2][2] = m.c3; r[3][2] = m.c4;
    r[0][3] = m.d1; r[1][3] = m.d2; r[2][3] = m.d3; r[3][3] = m.d4;
    return r;
}

static aiVector3D CST_LerpVec(const aiVector3D& a, const aiVector3D& b, float f)
{
    return a + (b - a) * f;
}

static unsigned CST_FindKey_Pos(const aiNodeAnim* ch, double t)
{
    for (unsigned i = 0; i + 1 < ch->mNumPositionKeys; i++)
        if (t < ch->mPositionKeys[i + 1].mTime) return i;
    return 0;
}
static unsigned CST_FindKey_Scl(const aiNodeAnim* ch, double t)
{
    for (unsigned i = 0; i + 1 < ch->mNumScalingKeys; i++)
        if (t < ch->mScalingKeys[i + 1].mTime) return i;
    return 0;
}
static unsigned CST_FindKey_Rot(const aiNodeAnim* ch, double t)
{
    for (unsigned i = 0; i + 1 < ch->mNumRotationKeys; i++)
        if (t < ch->mRotationKeys[i + 1].mTime) return i;
    return 0;
}

static aiVector3D CST_InterpPos(const aiNodeAnim* ch, double t)
{
    if (ch->mNumPositionKeys == 1) return ch->mPositionKeys[0].mValue;
    unsigned i = CST_FindKey_Pos(ch, t);
    unsigned j = i + 1;
    double t0 = ch->mPositionKeys[i].mTime;
    double t1 = ch->mPositionKeys[j].mTime;
    float f = float((t - t0) / (t1 - t0));
    return CST_LerpVec(ch->mPositionKeys[i].mValue, ch->mPositionKeys[j].mValue, f);
}

static aiVector3D CST_InterpScl(const aiNodeAnim* ch, double t)
{
    if (ch->mNumScalingKeys == 1) return ch->mScalingKeys[0].mValue;
    unsigned i = CST_FindKey_Scl(ch, t);
    unsigned j = i + 1;
    double t0 = ch->mScalingKeys[i].mTime;
    double t1 = ch->mScalingKeys[j].mTime;
    float f = float((t - t0) / (t1 - t0));
    return CST_LerpVec(ch->mScalingKeys[i].mValue, ch->mScalingKeys[j].mValue, f);
}

static aiQuaternion CST_InterpRot(const aiNodeAnim* ch, double t)
{
    if (ch->mNumRotationKeys == 1) return ch->mRotationKeys[0].mValue;
    unsigned i = CST_FindKey_Rot(ch, t);
    unsigned j = i + 1;
    double t0 = ch->mRotationKeys[i].mTime;
    double t1 = ch->mRotationKeys[j].mTime;
    float f = float((t - t0) / (t1 - t0));

    aiQuaternion out;
    aiQuaternion::Interpolate(out, ch->mRotationKeys[i].mValue, ch->mRotationKeys[j].mValue, f);
    out.Normalize();
    return out;
}

// ===================== skinning =====================

static const int CST_MAX_BONES = 128;

struct CST_Vertex
{
    glm::vec3 pos;
    glm::vec3 nrm;
    glm::vec2 uv;
    glm::ivec4 bone;   // 4 bone IDs
    glm::vec4 weight;  // 4 weights
};

static void CST_ResetVertex(CST_Vertex& v)
{
    v.pos = glm::vec3(0);
    v.nrm = glm::vec3(0, 1, 0);
    v.uv = glm::vec2(0);
    v.bone = glm::ivec4(0);
    v.weight = glm::vec4(0.0f);
}

static void CST_AddBoneData(CST_Vertex& v, int boneId, float w)
{
    for (int i = 0; i < 4; ++i)
    {
        if (v.weight[i] == 0.0f)
        {
            v.bone[i] = boneId;
            v.weight[i] = w;
            return;
        }
    }
    int minI = 0;
    for (int i = 1; i < 4; ++i)
        if (v.weight[i] < v.weight[minI]) minI = i;

    if (w > v.weight[minI])
    {
        v.bone[minI] = boneId;
        v.weight[minI] = w;
    }
}

struct CST_BoneInfo
{
    std::string name;
    glm::mat4 offset;   // inverse bind
    int id = -1;
};

struct CST_Mesh
{
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
    std::string nodeName;
    GLuint baseColorTex = 0;

    std::vector<CST_BoneInfo> bones;
    std::unordered_map<std::string, int> boneNameToId;

    void Draw() const
    {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }
};

struct ChainsawTest
{
    Assimp::Importer importer;
    const aiScene* scene = nullptr;

    std::vector<CST_Mesh> meshes;

    float animTime = 0.f;
    std::unordered_map<std::string, const aiNodeAnim*> channels;

    std::unordered_map<std::string, aiMatrix4x4> bindGlobal;
    std::unordered_map<std::string, aiMatrix4x4> animGlobal;

    aiMatrix4x4 globalInv;

    bool Load(const char* path)
    {
        scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_LimitBoneWeights);

        if (!scene) return false;

        // inverse(root)
        globalInv = scene->mRootNode->mTransformation;
        globalInv.Inverse();

        // channels
        if (scene->HasAnimations())
        {
            auto* anim = scene->mAnimations[0];
            for (unsigned i = 0; i < anim->mNumChannels; i++)
                channels[anim->mChannels[i]->mNodeName.C_Str()] = anim->mChannels[i];
        }

        // walk nodes
        std::function<void(aiNode*, aiMatrix4x4)> walk;
        walk = [&](aiNode* node, aiMatrix4x4 parent)
            {
                aiMatrix4x4 global = parent * node->mTransformation;
                bindGlobal[node->mName.C_Str()] = global;

                for (unsigned mi = 0; mi < node->mNumMeshes; mi++)
                {
                    aiMesh* m = scene->mMeshes[node->mMeshes[mi]];
                    if (!m) continue;

                    std::vector<CST_Vertex> verts;
                    std::vector<unsigned> idx;

                    verts.resize(m->mNumVertices);
                    for (unsigned k = 0; k < m->mNumVertices; k++)
                    {
                        CST_ResetVertex(verts[k]);

                        const aiVector3D& p = m->mVertices[k];
                        verts[k].pos = glm::vec3(p.x, p.y, p.z);

                        if (m->HasNormals())
                        {
                            const aiVector3D& n = m->mNormals[k];
                            verts[k].nrm = glm::vec3(n.x, n.y, n.z);
                        }

                        if (m->HasTextureCoords(0))
                        {
                            const aiVector3D& t = m->mTextureCoords[0][k];
                            verts[k].uv = glm::vec2(t.x, t.y);
                        }
                    }

                    // indices
                    idx.reserve(m->mNumFaces * 3);
                    for (unsigned f = 0; f < m->mNumFaces; f++)
                        for (unsigned j = 0; j < m->mFaces[f].mNumIndices; j++)
                            idx.push_back(m->mFaces[f].mIndices[j]);

                    CST_Mesh out;
                    out.indexCount = (GLsizei)idx.size();

                    // ВОТ ЭТА СТРОКА НУЖНА
                    out.nodeName = node->mName.C_Str();

                    // bones + weights
                    if (m->HasBones())
                    {
                        out.bones.reserve(m->mNumBones);

                        int nextId = 0;
                        for (unsigned b = 0; b < m->mNumBones && nextId < CST_MAX_BONES; ++b)
                        {
                            aiBone* bone = m->mBones[b];
                            if (!bone) continue;

                            std::string bname = bone->mName.C_Str();

                            CST_BoneInfo bi;
                            bi.name = bname;
                            bi.id = nextId++;
                            bi.offset = CST_AiToGlm(bone->mOffsetMatrix);

                            out.boneNameToId[bname] = bi.id;
                            out.bones.push_back(bi);

                            for (unsigned w = 0; w < bone->mNumWeights; ++w)
                            {
                                unsigned vid = bone->mWeights[w].mVertexId;
                                float ww = bone->mWeights[w].mWeight;
                                if (vid < verts.size())
                                    CST_AddBoneData(verts[vid], bi.id, ww);
                            }
                        }
                    }

                    // base color texture (самый простой вариант: пытаемся взять DIFFUSE/BASE_COLOR путь)
                    if (m->mMaterialIndex < scene->mNumMaterials)
                    {
                        aiMaterial* mat = scene->mMaterials[m->mMaterialIndex];
                        aiString texPath;

                        if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
                            mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
                        {
                            const char* p = texPath.C_Str();
                            // если glb embedded "*0" — пока пропускаем, чтобы не тащить stb сюда
                            if (p && p[0] != '*')
                                out.baseColorTex = LoadTexture2D(p);
                        }
                    }

                    // GPU
                    glGenVertexArrays(1, &out.vao);
                    glGenBuffers(1, &out.vbo);
                    glGenBuffers(1, &out.ebo);

                    glBindVertexArray(out.vao);

                    glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
                    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(CST_Vertex), verts.data(), GL_STATIC_DRAW);

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);

                    GLsizei stride = (GLsizei)sizeof(CST_Vertex);

                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CST_Vertex, pos));

                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CST_Vertex, nrm));

                    glEnableVertexAttribArray(2);
                    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CST_Vertex, uv));

                    glEnableVertexAttribArray(3);
                    glVertexAttribIPointer(3, 4, GL_INT, stride, (void*)offsetof(CST_Vertex, bone));

                    glEnableVertexAttribArray(4);
                    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(CST_Vertex, weight));

                    glBindVertexArray(0);

                    meshes.push_back(out);
                }

                for (unsigned c = 0; c < node->mNumChildren; c++)
                    walk(node->mChildren[c], global);
            };

        walk(scene->mRootNode, aiMatrix4x4());
        return true;
    }

    void Update(float dt)
    {
        if (!scene || !scene->HasAnimations()) return;

        auto* anim = scene->mAnimations[0];
        double tps = anim->mTicksPerSecond ? anim->mTicksPerSecond : 25.0;

        animTime += dt;
        double ticks = animTime * tps;
        double time = fmod(ticks, anim->mDuration);

        animGlobal.clear();

        std::function<void(aiNode*, aiMatrix4x4)> eval;
        eval = [&](aiNode* node, aiMatrix4x4 parent)
            {
                aiMatrix4x4 local = node->mTransformation;

                auto it = channels.find(node->mName.C_Str());
                if (it != channels.end())
                {
                    auto* ch = it->second;

                    auto p = CST_InterpPos(ch, time);
                    auto s = CST_InterpScl(ch, time);
                    auto r = CST_InterpRot(ch, time);

                    aiMatrix4x4 T; aiMatrix4x4::Translation(p, T);
                    aiMatrix4x4 R(r.GetMatrix());
                    aiMatrix4x4 S; aiMatrix4x4::Scaling(s, S);

                    local = T * R * S;
                }

                aiMatrix4x4 g = parent * local;
                animGlobal[node->mName.C_Str()] = g;

                for (unsigned c = 0; c < node->mNumChildren; c++)
                    eval(node->mChildren[c], g);
            };

        eval(scene->mRootNode, aiMatrix4x4());
    }

    glm::mat4 BoneFinal(const std::string& boneName, const glm::mat4& offset) const
    {
        aiMatrix4x4 g;
        bool has = false;

        auto ita = animGlobal.find(boneName);
        if (ita != animGlobal.end())
        {
            g = ita->second;
            has = true;
        }
        if (!has)
        {
            auto itb = bindGlobal.find(boneName);
            if (itb != bindGlobal.end())
            {
                g = itb->second;
                has = true;
            }
        }
        if (!has)
        {
            g = aiMatrix4x4(); // identity
        }

        aiMatrix4x4 fin = globalInv * g;
        return CST_AiToGlm(fin) * offset;
    }
};

// ===================== global test unit =====================

static ChainsawTest g_chainsawTest;
static GLuint g_chainsawShader = 0;

inline void InitChainsawTest()
{
    g_chainsawShader = CreateShaderProgram("chainsaw_test.vert", "chainsaw_test.frag");

    if (!g_chainsawTest.Load("chainsaw.glb"))
        OutputDebugStringA("ChainsawTest: load failed\n");
    else
        OutputDebugStringA("ChainsawTest: load OK\n");
}

inline void UpdateChainsawTest(float dt)
{
    if (g_currentTool != 3) return; // TOOL_CHAINSAW_TEST
    g_chainsawTest.Update(dt);
}

inline void DrawChainsawTestViewModel(const glm::mat4& proj, const glm::mat4& view)
{
    if (g_currentTool != 3) return;
    if (!g_chainsawShader || g_chainsawTest.meshes.empty()) return;

    glUseProgram(g_chainsawShader);

    glUniformMatrix4fv(glGetUniformLocation(g_chainsawShader, "uProjection"), 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(g_chainsawShader, "uView"), 1, GL_FALSE, &view[0][0]);

    glm::vec3 lightDir = glm::normalize(glm::vec3(0.4f, 1.0f, 0.2f));
    glUniform3fv(glGetUniformLocation(g_chainsawShader, "uLightDir"), 1, &lightDir[0]);

    glm::mat4 local(1.0f);
    local = glm::translate(local, glm::vec3(0.55f, -1.45f, -3.2f));
    local = glm::rotate(local, glm::radians(-10.0f), glm::vec3(1, 0, 0));
    local = glm::rotate(local, glm::radians(12.0f), glm::vec3(0, 1, 0));
    local = glm::scale(local, glm::vec3(0.55f));

    glm::mat4 camMatrix = glm::inverse(view);
    glm::mat4 model = camMatrix * local;
    glUniformMatrix4fv(glGetUniformLocation(g_chainsawShader, "uModel"), 1, GL_FALSE, &model[0][0]);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0, 0.1);

    GLint locNode = glGetUniformLocation(g_chainsawShader, "uNode");
    GLint locBones = glGetUniformLocation(g_chainsawShader, "uBones");
    GLint locHasTex = glGetUniformLocation(g_chainsawShader, "uHasTex");
    GLint locTex = glGetUniformLocation(g_chainsawShader, "uTex");

    for (const auto& mesh : g_chainsawTest.meshes)
    {
        // --- uNode: если меш БЕЗ костей, даём анимацию узла; если С костями — identity (чтобы не удваивать)
        glm::mat4 nodeM(1.0f);
        if (mesh.bones.empty())
        {
            // берём animGlobal, если есть, иначе bindGlobal
            auto ita = g_chainsawTest.animGlobal.find(mesh.nodeName);
            if (ita != g_chainsawTest.animGlobal.end())
                nodeM = CST_AiToGlm(ita->second);
            else
            {
                auto itb = g_chainsawTest.bindGlobal.find(mesh.nodeName);
                if (itb != g_chainsawTest.bindGlobal.end())
                    nodeM = CST_AiToGlm(itb->second);
            }
        }
        if (locNode >= 0) glUniformMatrix4fv(locNode, 1, GL_FALSE, &nodeM[0][0]);

        // --- texture
        if (mesh.baseColorTex)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.baseColorTex);
            if (locTex >= 0) glUniform1i(locTex, 0);
            if (locHasTex >= 0) glUniform1i(locHasTex, 1);
        }
        else
        {
            if (locHasTex >= 0) glUniform1i(locHasTex, 0);
        }

        // --- bones
        if (locBones >= 0)
        {
            std::array<glm::mat4, CST_MAX_BONES> bonesM;
            for (int i = 0; i < CST_MAX_BONES; ++i) bonesM[i] = glm::mat4(1.0f);

            for (const auto& b : mesh.bones)
            {
                if (b.id >= 0 && b.id < CST_MAX_BONES)
                    bonesM[b.id] = g_chainsawTest.BoneFinal(b.name, b.offset);
            }

            glUniformMatrix4fv(locBones, CST_MAX_BONES, GL_FALSE, &bonesM[0][0][0]);
        }

        mesh.Draw();
    }

    glDepthRange(0.0, 1.0);
}
