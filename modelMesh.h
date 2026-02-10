#pragma once
#include <vector>
#include <string>

#include <GL/glew.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class ModelMesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;

    ModelMesh(std::vector<Vertex> vertices,
        std::vector<unsigned int> indices,
        std::vector<Texture> textures);

    void Draw(unsigned int shaderID);

private:
    unsigned int VBO, EBO;
    void setupMesh();
};
