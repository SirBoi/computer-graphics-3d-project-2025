#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> 


struct Mesh {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei indexCount = 0;
};
Mesh createSandGrid(int nx, int nz, float x0, float x1, float z0, float z1, float y, float uvTile);
Mesh createSphere(int stacks, int slices, float radius);

struct PointLight {
    glm::vec3 pos;
    glm::vec3 color;
    float intensity;
};
void uploadPointLights(GLuint program, const std::vector<PointLight>& lights);
void uploadViewPos(GLuint program, const glm::vec3& viewPos);

int endProgram(std::string message);
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
GLFWcursor* loadImageToCursor(const char* filePath);
bool hitSphere(const glm::vec3& a, float ar, const glm::vec3& b, float br);