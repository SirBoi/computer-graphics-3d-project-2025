#include "model.h"

#include "stb_image.h"
#include <iostream>

Model::Model(const std::string& path)
{
    loadModel(path);
}

Model::Model(const std::string& path, const std::string& keepNodeNameSubstring)
    : keepSubstr(keepNodeNameSubstring) {
    loadModel(path);
}

void Model::Draw(unsigned int shaderID)
{
    for (auto& mesh : meshes)
        mesh.Draw(shaderID);
}

void Model::loadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals
    );

    printNode(scene->mRootNode, scene, 0);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        return;

    directory = path.substr(0, path.find_last_of("/\\"));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    bool keepThis = shouldKeepNode(node->mName); // your substring check

    // Only add this node's meshes if it matches
    if (keepThis) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
    }

    // Always traverse children (IMPORTANT)
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}


ModelMesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // vertices
    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v;

        v.Position = glm::vec3(mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);

        if (mesh->mNormals) {
            v.Normal = glm::vec3(mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z);
        }
        else {
            v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (mesh->HasTextureCoords(0)) {
            v.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y);
        }
        else {
            v.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(v);
    }

    // indices
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // diffuse texture (material)
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

        aiString str;
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &str) == AI_SUCCESS)
        {
            Texture tex;
            tex.id = loadTexture(str.C_Str());
            tex.type = "diffuseTex";          // <-- IMPORTANT: match shader uniform name
            tex.path = str.C_Str();           // optional, but useful for debugging
            textures.push_back(tex);
        }
    }

    return ModelMesh(vertices, indices, textures);
}


unsigned int Model::loadTexture(const char* path)
{
    if (!path || path[0] == '\0') {
        std::cout << "[loadTexture] empty path\n";
        return 0;
    }

    std::string p(path);

    // trim quotes
    if (!p.empty() && (p.front() == '"' || p.front() == '\'')) p.erase(p.begin());
    if (!p.empty() && (p.back() == '"' || p.back() == '\'')) p.pop_back();

    // normalize slashes
    for (char& c : p) if (c == '\\') c = '/';

    std::string filename = directory + "/" + p;

    // IMPORTANT: flush so you see it even if you crash immediately after
    std::cout << "[loadTexture] trying: " << filename << std::endl;

    int w = 0, h = 0, n = 0;
    unsigned char* data = stbi_load(filename.c_str(), &w, &h, &n, 0);

    unsigned int texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    GLenum format =
        (n == 4) ? GL_RGBA :
        (n == 3) ? GL_RGB :
        (n == 1) ? GL_RED : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    std::cout << "[loadTexture] OK id=" << texID << " (" << w << "x" << h << ")\n" << std::endl;
    return texID;
}


void Model::printNode(aiNode* node, const aiScene* scene, int depth) {
    for (int i = 0; i < depth; i++) std::cout << "  ";
    std::cout << node->mName.C_Str() << " meshes=" << node->mNumMeshes << "\n";
    for (unsigned i = 0; i < node->mNumMeshes; i++) {
        unsigned mi = node->mMeshes[i];
        std::cout << std::string(depth * 2, ' ')
            << "  mesh[" << mi << "] verts=" << scene->mMeshes[mi]->mNumVertices << "\n";
    }
    for (unsigned i = 0; i < node->mNumChildren; i++) printNode(node->mChildren[i], scene, depth + 1);
}

bool Model::shouldKeepNode(const aiString& name) const {
    if (keepSubstr.empty()) return true;
    const char* n = name.C_Str();
    if (!n) return false;
    return std::string(n).find(keepSubstr) != std::string::npos;
}

std::vector<Texture> Model::loadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    const std::string& typeName)
{
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        Texture texture;
        texture.id = loadTexture(str.C_Str());
        texture.type = typeName;
        texture.path = str.C_Str();

        textures.push_back(texture);
    }
    return textures;
}
