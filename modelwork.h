#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct TextureInfo {
    GLuint id;
    std::string type;   // "texture_diffuse", "texture_specular", ...
    std::string path;   // дл€ предотвращени€ повторной загрузки
};

static TextureInfo LoadTextureFromFile(const std::string& filename,
    const std::string& directory,
    const std::string& typeName,
    std::vector<TextureInfo>& loaded)
{
    std::string fullPath = directory + "/" + filename;

    // провер€ем кэш
    for (auto& t : loaded) {
        if (t.path == fullPath && t.type == typeName)
            return t;
    }

    TextureInfo tex;
    tex.id = LoadTexture2D(fullPath.c_str());
    tex.type = typeName;
    tex.path = fullPath;

    loaded.push_back(tex);
    return tex;
}

TextureInfo LoadTexture_Assimp(
    const aiScene* scene,
    const aiMaterial* material,
    aiTextureType type,
    unsigned int index,
    const std::string& directory,
    const std::string& typeName,
    std::vector<TextureInfo>& loaded // чтобы не дублировать
)
{
    aiString str;
    if (material->GetTexture(type, index, &str) != AI_SUCCESS)
        return { 0, "", "" };

    std::string texPath = str.C_Str();

    // 1) ”же грузили такую?
    for (const auto& t : loaded) {
        if (t.path == texPath && t.type == typeName)
            return t;
    }

    TextureInfo tex{};
    tex.type = typeName;
    tex.path = texPath;

    // 2) Embedded texture: путь типа "*0"
    if (!texPath.empty() && texPath[0] == '*')
    {
        int texIndex = std::atoi(texPath.c_str() + 1);
        if (texIndex >= 0 && texIndex < (int)scene->mNumTextures)
        {
            const aiTexture* aitex = scene->mTextures[texIndex];

            GLuint id = 0;
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            int w = 0, h = 0, ch = 0;

            if (aitex->mHeight == 0)
            {
                // сжатый формат (PNG/JPEG) в пам€ти
                const unsigned char* data = reinterpret_cast<const unsigned char*>(aitex->pcData);
                int size = (int)aitex->mWidth;

                unsigned char* img = stbi_load_from_memory(data, size, &w, &h, &ch, 4);
                if (img)
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    stbi_image_free(img);
                    tex.id = id;
                }
                else
                {
                    glDeleteTextures(1, &id);
                }
            }
            else
            {
                // несжатый BGRA8888
                w = (int)aitex->mWidth;
                h = (int)aitex->mHeight;
                std::vector<unsigned char> pixels(w * h * 4);

                const unsigned char* src = reinterpret_cast<const unsigned char*>(aitex->pcData);
                // Assimp хранит в aiTexel, который уже uint32 BGRA
                for (int i = 0; i < w * h; ++i)
                {
                    pixels[i * 4 + 0] = src[i * 4 + 2]; // B->R
                    pixels[i * 4 + 1] = src[i * 4 + 1]; // G
                    pixels[i * 4 + 2] = src[i * 4 + 0]; // R->B
                    pixels[i * 4 + 3] = src[i * 4 + 3]; // A
                }

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                glGenerateMipmap(GL_TEXTURE_2D);
                tex.id = id;
            }

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    else
    {
        // 3) ќбычный внешний файл (как было дл€ OBJ)
        std::string filename = texPath;
        if (!directory.empty())
            filename = directory + "/" + filename;

        int w, h, ch;
        unsigned char* data = stbi_load(filename.c_str(), &w, &h, &ch, 4);
        if (data)
        {
            GLuint id;
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            glBindTexture(GL_TEXTURE_2D, 0);

            tex.id = id;
        }
    }

    if (tex.id != 0)
        loaded.push_back(tex);

    return tex;
}


struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
    std::vector<TextureInfo> textures;

    std::string nodeName;
    glm::mat4 bindNodeTransform = glm::mat4(1.0f);

    void Draw(GLuint shader) const
    {
        if (!textures.empty()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0].id);
            GLint loc = glGetUniformLocation(shader, "uTex");
            if (loc >= 0)
                glUniform1i(loc, 0);
        }

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
    }
    void DrawInstanced(GLuint shader, GLsizei instanceCount) const
    {
        if (!textures.empty()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0].id);
            GLint loc = glGetUniformLocation(shader, "uTex");
            if (loc >= 0)
                glUniform1i(loc, 0);
        }

        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, instanceCount);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
    }
};

glm::mat4 AiToGlm(const aiMatrix4x4& m)
{
    // Assimp -> GLM (column-major)
    return glm::mat4(
        m.a1, m.a2, m.a3, m.a4,
        m.b1, m.b2, m.b3, m.b4,
        m.c1, m.c2, m.c3, m.c4,
        m.d1, m.d2, m.d3, m.d4
    );
}

struct Model {

    float animTime = 0.0f;

    std::unordered_map<std::string, glm::mat4> animatedGlobal; // глобальные матрицы узлов (текущий кадр)
    std::unordered_map<std::string, const aiNodeAnim*> channels; // nodeName -> канал анимации

    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<TextureInfo> loadedTextures; // кэш

    std::unique_ptr<Assimp::Importer> importer;      // <-- ƒќЅј¬»“№
    const aiScene* scene = nullptr; // <-- ƒќЅј¬»“№

    bool Load(const std::string& path);
    void Draw(GLuint shader) const;
    void DrawInstanced(GLuint shader, GLsizei instanceCount) const;
    void UpdateAnimation(float dt);
};

struct TreeInstance {
    glm::vec3 pos;
    float     scale;
    float     radius; // дл€ коллизии
};


Model g_treeModel;
GLuint g_treeInstanceVBO = 0;
GLsizei g_treeInstanceCount = 0;
std::vector<TreeInstance> g_treeInstances;
GLuint g_treeShader = 0;

bool LoadOBJ(const char* path, Mesh& outMesh);
void InitTreeObjects();
void DrawTreeObjects(const glm::mat4& proj, const glm::mat4& view);
void ResolveTreeCollisions(glm::vec3& pos);





bool Model::Load(const std::string& path)
{
    // —брасываем данные модели
    meshes.clear();
    loadedTextures.clear();
    directory = path.substr(0, path.find_last_of("/\\"));

    // ѕересоздаЄм importer (это фиксит DeadlyImportError и "плавающее" состо€ние)
    importer.reset(new Assimp::Importer());

    try
    {
        scene = importer->ReadFile(
            path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_SortByPType |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph |
            aiProcess_FlipUVs
        );
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA(("ASSIMP exception: " + std::string(e.what()) + "\n").c_str());
        scene = nullptr;
        return false;
    }

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
    {
        OutputDebugStringA(("ASSIMP error: " + std::string(importer->GetErrorString()) + "\n").c_str());
        return false;
    }

    OutputDebugStringA(scene->HasAnimations() ? "GLB: has animations\n" : "GLB: no animations\n");
    for (unsigned i = 0; i < scene->mNumMeshes; ++i)
        if (scene->mMeshes[i]->HasBones()) OutputDebugStringA("GLB: mesh has bones\n");

    channels.clear();
    animatedGlobal.clear();
    animTime = 0.0f;

    if (scene->HasAnimations() && scene->mNumAnimations > 0) {
        aiAnimation* anim = scene->mAnimations[0];
        for (unsigned i = 0; i < anim->mNumChannels; ++i) {
            const aiNodeAnim* ch = anim->mChannels[i];
            channels[ch->mNodeName.C_Str()] = ch;
        }
    }


    //// сохран€ем директорию
    //directory = path.substr(0, path.find_last_of("/\\"));
    //meshes.clear();
    //loadedTextures.clear();

    std::function<void(aiNode*, const aiScene*, const aiMatrix4x4&)> processNode;
    processNode = [&](aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform)
      {
            aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;
            for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

                std::vector<float> vertices;
                std::vector<unsigned int> indices;
                std::vector<TextureInfo> textures;

                vertices.reserve(mesh->mNumVertices * 8);

              /*  glm::mat4 nodeMat = AiToGlm(nodeTransform);
                glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(nodeMat)));*/

                for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                    aiVector3D pos = mesh->mVertices[v];
                    aiVector3D nor = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0, 1, 0);
                    aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][v] : aiVector3D(0, 0, 0);

                    glm::vec3 gpos = glm::vec3(pos.x, pos.y, pos.z);
                    glm::vec3 gnor = glm::vec3(nor.x, nor.y, nor.z);

                    vertices.push_back(gpos.x);
                    vertices.push_back(gpos.y);
                    vertices.push_back(gpos.z);

                    vertices.push_back(gnor.x);
                    vertices.push_back(gnor.y);
                    vertices.push_back(gnor.z);

                    vertices.push_back(uv.x);
                    vertices.push_back(uv.y);
                }

                for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                    const aiFace& face = mesh->mFaces[f];
                    for (unsigned int j = 0; j < face.mNumIndices; ++j)
                        indices.push_back(face.mIndices[j]);
                }

                // материалы
                if (mesh->mMaterialIndex >= 0)
                {
                    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

                    // glTF/GLB может класть базовый цвет и как DIFFUSE, и как BASE_COLOR
                    auto texDiffuse = LoadTexture_Assimp(scene, material,
                        aiTextureType_DIFFUSE,
                        0, directory,
                        "texture_diffuse", textures);
                    if (texDiffuse.id == 0)
                    {
                        texDiffuse = LoadTexture_Assimp(scene, material,
                            (aiTextureType)aiTextureType_BASE_COLOR,
                            0, directory,
                            "texture_diffuse", textures);
                    }
                }

                Mesh out;
                out.textures = textures;
                out.indexCount = (GLsizei)indices.size();

                glGenVertexArrays(1, &out.vao);
                glGenBuffers(1, &out.vbo);
                glGenBuffers(1, &out.ebo);

                glBindVertexArray(out.vao);

                glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
                glBufferData(GL_ARRAY_BUFFER,
                    vertices.size() * sizeof(float),
                    vertices.data(),
                    GL_STATIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    indices.size() * sizeof(unsigned int),
                    indices.data(),
                    GL_STATIC_DRAW);

                GLsizei stride = 8 * sizeof(float);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

                glBindVertexArray(0);

                out.nodeName = node->mName.C_Str();
                out.bindNodeTransform = AiToGlm(nodeTransform);

                meshes.push_back(out);
            }

            for (unsigned int i = 0; i < node->mNumChildren; ++i)
                processNode(node->mChildren[i], scene, nodeTransform);
        };

    processNode(scene->mRootNode, scene, aiMatrix4x4());

    return !meshes.empty();
}

void Model::Draw(GLuint shader) const
{
    for (const auto& m : meshes)
    {
        // 1) ЅерЄм матрицу узла: либо анимированную, либо bind
        glm::mat4 nodeM = m.bindNodeTransform;

        auto it = animatedGlobal.find(m.nodeName);
        if (it != animatedGlobal.end())
            nodeM = it->second;

        // 2) ќтправл€ем в шейдер
        GLint loc = glGetUniformLocation(shader, "uModel");
        if (loc >= 0)
            glUniformMatrix4fv(loc, 1, GL_FALSE, &nodeM[0][0]);

        // 3) –исуем меш
        m.Draw(shader);
    }
}

void Model::DrawInstanced(GLuint shader, GLsizei instanceCount) const
{
    for (const auto& m : meshes)
        m.DrawInstanced(shader, instanceCount);
}

static unsigned FindKeyIndex_Pos(const aiNodeAnim* ch, double t)
{
    for (unsigned i = 0; i + 1 < ch->mNumPositionKeys; ++i)
        if (t < ch->mPositionKeys[i + 1].mTime) return i;
    return (ch->mNumPositionKeys > 0) ? (ch->mNumPositionKeys - 1) : 0;
}

static unsigned FindKeyIndex_Scale(const aiNodeAnim* ch, double t)
{
    for (unsigned i = 0; i + 1 < ch->mNumScalingKeys; ++i)
        if (t < ch->mScalingKeys[i + 1].mTime) return i;
    return (ch->mNumScalingKeys > 0) ? (ch->mNumScalingKeys - 1) : 0;
}

static unsigned FindKeyIndex_Rot(const aiNodeAnim* ch, double t)
{
    for (unsigned i = 0; i + 1 < ch->mNumRotationKeys; ++i)
        if (t < ch->mRotationKeys[i + 1].mTime) return i;
    return (ch->mNumRotationKeys > 0) ? (ch->mNumRotationKeys - 1) : 0;
}

static aiVector3D InterpPos(const aiNodeAnim* ch, double t)
{
    if (!ch || ch->mNumPositionKeys == 0) return aiVector3D(0, 0, 0);
    if (ch->mNumPositionKeys == 1) return ch->mPositionKeys[0].mValue;

    unsigned i = FindKeyIndex_Pos(ch, t);
    unsigned j = (i + 1 < ch->mNumPositionKeys) ? i + 1 : i;

    double t0 = ch->mPositionKeys[i].mTime;
    double t1 = ch->mPositionKeys[j].mTime;
    if (t1 <= t0) return ch->mPositionKeys[i].mValue;

    float f = (float)((t - t0) / (t1 - t0));
    const aiVector3D& a = ch->mPositionKeys[i].mValue;
    const aiVector3D& b = ch->mPositionKeys[j].mValue;
    return a + (b - a) * f;
}

static aiVector3D InterpScale(const aiNodeAnim* ch, double t)
{
    if (!ch || ch->mNumScalingKeys == 0) return aiVector3D(1, 1, 1);
    if (ch->mNumScalingKeys == 1) return ch->mScalingKeys[0].mValue;

    unsigned i = FindKeyIndex_Scale(ch, t);
    unsigned j = (i + 1 < ch->mNumScalingKeys) ? i + 1 : i;

    double t0 = ch->mScalingKeys[i].mTime;
    double t1 = ch->mScalingKeys[j].mTime;
    if (t1 <= t0) return ch->mScalingKeys[i].mValue;

    float f = (float)((t - t0) / (t1 - t0));
    const aiVector3D& a = ch->mScalingKeys[i].mValue;
    const aiVector3D& b = ch->mScalingKeys[j].mValue;
    return a + (b - a) * f;
}

static aiQuaternion InterpRot(const aiNodeAnim* ch, double t)
{
    if (!ch || ch->mNumRotationKeys == 0) return aiQuaternion(1, 0, 0, 0);
    if (ch->mNumRotationKeys == 1) return ch->mRotationKeys[0].mValue;

    unsigned i = FindKeyIndex_Rot(ch, t);
    unsigned j = (i + 1 < ch->mNumRotationKeys) ? i + 1 : i;

    double t0 = ch->mRotationKeys[i].mTime;
    double t1 = ch->mRotationKeys[j].mTime;
    if (t1 <= t0) return ch->mRotationKeys[i].mValue;

    float f = (float)((t - t0) / (t1 - t0));

    aiQuaternion out;
    aiQuaternion::Interpolate(out, ch->mRotationKeys[i].mValue, ch->mRotationKeys[j].mValue, f);
    out.Normalize();
    return out;
}

void Model::UpdateAnimation(float dt)
{
    if (!scene || !scene->HasAnimations() || scene->mNumAnimations == 0) return;

    aiAnimation* anim = scene->mAnimations[0];
    double tps = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;
    double duration = anim->mDuration;

    animTime += dt;
    double timeInTicks = fmod(animTime * tps, duration);

    // –екурсивно считаем global transform дл€ каждого узла
    std::function<void(aiNode*, const aiMatrix4x4&)> evalNode;
    evalNode = [&](aiNode* node, const aiMatrix4x4& parent)
        {
            aiMatrix4x4 local = node->mTransformation;

            auto it = channels.find(node->mName.C_Str());
            if (it != channels.end())
            {
                const aiNodeAnim* ch = it->second;

                aiVector3D p = InterpPos(ch, timeInTicks);
                aiVector3D s = InterpScale(ch, timeInTicks);
                aiQuaternion r = InterpRot(ch, timeInTicks);

                aiMatrix4x4 mS; aiMatrix4x4::Scaling(s, mS);
                aiMatrix4x4 mR = aiMatrix4x4(r.GetMatrix());
                aiMatrix4x4 mT; aiMatrix4x4::Translation(p, mT);

                local = mT * mR * mS;
            }

            aiMatrix4x4 global = parent * local;
            animatedGlobal[node->mName.C_Str()] = AiToGlm(global);

            for (unsigned i = 0; i < node->mNumChildren; ++i)
                evalNode(node->mChildren[i], global);
        };

    evalNode(scene->mRootNode, aiMatrix4x4());
}