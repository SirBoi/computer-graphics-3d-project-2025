#pragma once
#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "modelMesh.h"

class Model {
public:
    Model(const std::string& path);
    Model(const std::string& path, const std::string& keepNodeNameSubstring);
    void Draw(unsigned int shaderID);

private:
    std::vector<ModelMesh> meshes;
    std::string directory;

    std::string keepSubstr;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    ModelMesh processMesh(aiMesh* mesh, const aiScene* scene);
    unsigned int loadTexture(const char* path);
    void printNode(aiNode* node, const aiScene* scene, int depth);
    bool shouldKeepNode(const aiString& name) const;
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
};
