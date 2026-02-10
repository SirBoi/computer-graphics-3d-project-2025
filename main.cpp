#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Util.h"
#include "aquariumFrame.h"
#include "treasureChest.h"
#include "model.hpp"
#include "shader.h"


#define FRAME_LIMIT 75


// Default values
int screenWidth = 800;
int screenHeight = 800;


struct BubbleInstance {
    glm::vec3 pos;
    glm::vec3 vel;
    float radius;
    float alpha;
    float killY;
    float seed;
};

struct FoodInstance {
    glm::vec3 pos;
    glm::vec3 vel;
    float radius;
    float killY;
    bool settled;
};


bool useTex = true;
bool transparent = true;

glm::vec3 clownfishOffset(-0.5f, 0.3f, -0.3f);
float clownfishSpeed = 0.015f;
glm::vec3 clownfishForward(1.0f, 0.0f, 0.0f);
glm::vec3 clownfishForwardDyn = glm::vec3(1.0f, 0.0f, 0.0f);
float clownfishScale = 10.0f;
const float clownfishBaseScale = 10.0f;
const float clownfishBaseRadius = 0.25f;

glm::vec3 goldfishOffset(0.6f, -0.1f, 0.1f);
float goldfishSpeed = 0.015f;
glm::vec3 goldfishForward(1.0f, 0.0f, 0.0f);
glm::vec3 goldfishForwardDyn = glm::vec3(1.0f, 0.0f, 0.0f);
float goldfishScale = 5.0f;
const float goldfishBaseScale = 5.0f;
const float goldfishBaseRadius = 0.25f;

glm::vec3 clownfishPrevPos = clownfishOffset;
glm::vec3 goldfishPrevPos = goldfishOffset;

glm::vec3 clownfishVelSmoothed(0.0f);
glm::vec3 goldfishVelSmoothed(0.0f);

glm::quat clownfishRot = glm::quat(1, 0, 0, 0);
glm::quat goldfishRot = glm::quat(1, 0, 0, 0);

const float ROT_FOLLOW_SPEED = 8.5f;
const float MIN_SPEED = 0.0005f;

std::vector<BubbleInstance> bubbles;
std::vector<FoodInstance> foods;

static unsigned int chestBodyVAO = 0, chestBodyVBO = 0;
static unsigned int chestLidVAO = 0, chestLidVBO = 0;

static unsigned int chestBodyAtlasTex[6];
static unsigned int chestLidAtlasTex[6];

static const int CHEST_STRIDE_FLOATS = 5;
static const int CHEST_VERTEX_COUNT = 36;

const float* chestBodyVertices = nullptr;
const float* chestLidVertices = nullptr;

bool chestOpen = false;
float chestLidAngle = 0.0f;
float chestLidTarget = 0.0f;
const float CHEST_LID_MAX = 45.0f;
const float CHEST_LID_SPEED = 120.0f;
glm::vec3 lidHingeLocal(0.0f);
bool lidHingeReady = false;

bool chestLightOn = false;
float chestLightValue = 0.0f;
float chestLightTarget = 0.0f;
const float CHEST_LIGHT_FADE_SPEED = 2.5f;
glm::vec3 chestLightLocal(0.0f, 0.10f, 0.00f);
glm::vec3 chestLightColor(1.0f, 0.35f, 0.05f);
float chestLightIntensity = 1.0f;

const float AQ_X_MIN = -1.6f;
const float AQ_X_MAX = 1.6f;
const float AQ_Z_MIN = -0.7f;
const float AQ_Z_MAX = 0.7f;
const float AQ_Y_MIN = -1.0f;
const float AQ_Y_MAX = 0.85f;


static float frand01() { return (float)rand() / (float)RAND_MAX; }
static float frand(float a, float b) { return a + (b - a) * frand01(); }

static void resolveSphereBounds(glm::vec3& p, float r)
{
    if (p.x < AQ_X_MIN + r) p.x = AQ_X_MIN + r;
    if (p.x > AQ_X_MAX - r) p.x = AQ_X_MAX - r;

    if (p.y < AQ_Y_MIN + r) p.y = AQ_Y_MIN + r;
    if (p.y > AQ_Y_MAX - r) p.y = AQ_Y_MAX - r;

    if (p.z < AQ_Z_MIN + r) p.z = AQ_Z_MIN + r;
    if (p.z > AQ_Z_MAX - r) p.z = AQ_Z_MAX - r;
}

static void resolveSphereSphere(glm::vec3& a, float ra, glm::vec3& b, float rb)
{
    glm::vec3 d = a - b;
    float dist2 = glm::dot(d, d);
    float minDist = ra + rb;

    if (dist2 < 1e-8f) {
        d = glm::vec3(1, 0, 0);
        dist2 = 1.0f;
    }

    if (dist2 < minDist * minDist) {
        float dist = sqrtf(dist2);
        glm::vec3 n = d / dist;
        float penetration = (minDist - dist);

        a += n * (penetration * 0.5f);
        b -= n * (penetration * 0.5f);
    }
}

void spawnFishBubbles(const glm::vec3& fishPos, const glm::vec3& fishForward, float scaleFactor)
{
    glm::vec3 fwd = fishForward;
    if (glm::length2(fwd) < 1e-6f) fwd = glm::vec3(1, 0, 0);
    fwd = glm::normalize(fwd);

    float mouthForward = 0.35f * scaleFactor;
    float mouthUp = 0.20f * scaleFactor;

    glm::vec3 base = fishPos + fwd * mouthForward + glm::vec3(0.0f, mouthUp, 0.0f);

    for (int i = 0; i < 3; i++) {
        BubbleInstance b;
        b.pos = base + glm::vec3((frand01() - 0.5f) * 0.04f * scaleFactor,
            (frand01() - 0.5f) * 0.03f * scaleFactor,
            (frand01() - 0.5f) * 0.04f * scaleFactor);

        b.vel = glm::vec3(0.0f, 0.25f + 0.15f * frand01(), 0.0f);

        b.radius = (0.02f + 0.01f * frand01()) * (0.6f + 0.4f * scaleFactor);

        b.alpha = 0.1f;
        b.killY = 1.0f;
        b.seed = frand01() * 100.0f;

        bubbles.push_back(b);
    }
}

void spawnFood()
{
    FoodInstance f;

    float xMin = -1.6f, xMax = 1.6f;
    float zMin = -0.7f, zMax = 0.7f;

    f.pos = glm::vec3(frand(xMin, xMax), 3.0f, frand(zMin, zMax));
    f.vel = glm::vec3(0.0f, -0.1f, 0.0f);
    f.radius = 0.015f;
    f.settled = false;
    f.killY = -5.0f;

    foods.push_back(f);
}

unsigned int preprocessTexture(const char* filepath) {
    unsigned int texture = loadImageToTexture(filepath);
    glBindTexture(GL_TEXTURE_2D, texture);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        chestOpen = !chestOpen;
        chestLidTarget = chestOpen ? CHEST_LID_MAX : 0.0f;
        chestLightTarget = chestOpen ? 1.0f : 0.0f;
    }
    if (key == GLFW_KEY_J && action == GLFW_PRESS) {
        spawnFishBubbles(clownfishOffset, clownfishForwardDyn, clownfishScale / clownfishBaseScale);
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        spawnFishBubbles(goldfishOffset, goldfishForwardDyn, goldfishScale / goldfishBaseScale);
    }
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        spawnFood();
    }
}

bool processInput(GLFWwindow* window) {
    // Clownfish
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) clownfishOffset.z -= clownfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) clownfishOffset.z += clownfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) clownfishOffset.x -= clownfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) clownfishOffset.x += clownfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) clownfishOffset.y += clownfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) clownfishOffset.y -= clownfishSpeed;

    // Goldfish
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) goldfishOffset.z -= goldfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) goldfishOffset.z += goldfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) goldfishOffset.x -= goldfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) goldfishOffset.x += goldfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) goldfishOffset.y += goldfishSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) goldfishOffset.y -= goldfishSpeed;

    // Shutdown on ESC key press
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        return false;
    }

    return true;
}

int main(void)
{
    if (!glfwInit())
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    srand(time(NULL));
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);

    // Full screen mode
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;
    
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Akvarijum", monitor, NULL);
    if (window == NULL) return endProgram("ERROR: Prozor nije uspeo da se kreira.");
    
    glfwMakeContextCurrent(window);
    glViewport(0, 0, screenWidth, screenHeight);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Textures -----------------------------------------------------------------------------------------------------------------------------------------------------

    // Sand
    unsigned int sandTexture = preprocessTexture("resources/textures/sand.jpg");
    std::cout << "sandTexture id = " << sandTexture << std::endl;

    // Chest
    chestBodyAtlasTex[1] = preprocessTexture("resources/textures/chest/body_back.png");
    chestBodyAtlasTex[5] = preprocessTexture("resources/textures/chest/body_bottom.png");
    chestBodyAtlasTex[0] = preprocessTexture("resources/textures/chest/body_front.png");
    chestBodyAtlasTex[2] = preprocessTexture("resources/textures/chest/body_side.png");
    chestBodyAtlasTex[3] = preprocessTexture("resources/textures/chest/body_side.png");
    chestBodyAtlasTex[4] = preprocessTexture("resources/textures/chest/body_top.png");

    chestLidAtlasTex[1] = preprocessTexture("resources/textures/chest/lid_back.png");
    chestLidAtlasTex[5] = preprocessTexture("resources/textures/chest/lid_bottom.png");
    chestLidAtlasTex[0] = preprocessTexture("resources/textures/chest/lid_front.png");
    chestLidAtlasTex[2] = preprocessTexture("resources/textures/chest/lid_side.png");
    chestLidAtlasTex[3] = preprocessTexture("resources/textures/chest/lid_side.png");
    chestLidAtlasTex[4] = preprocessTexture("resources/textures/chest/lid_top.png");

    // Signature
    unsigned int signatureTexture = preprocessTexture("resources/textures/signature/signature.png");

    // Models -------------------------------------------------------------------------------------------------------------------------------------------------------    
    
    Model* clownfish = nullptr;
    std::cout << "Ucitavam model clownfish..." << std::endl;
    try {
        clownfish = new Model("resources/models/clownfish/Clownfish_Low_Poly.obj");
        if (clownfish) {
            std::cout << "clownfish meshes = " << clownfish->meshes.size() << "\n";
            for (size_t i = 0; i < clownfish->meshes.size(); i++) {
                std::cout << "mesh[" << i << "] vertices=" << clownfish->meshes[i].vertices.size()
                    << " indices=" << clownfish->meshes[i].indices.size()
                    << " textures=" << clownfish->meshes[i].textures.size() << "\n";
            }
        }
        std::cout << "Model uspesno ucitan!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "GRESKA pri ucitavanju modela: " << e.what() << std::endl;
        clownfish = nullptr;
    }

    Model* goldfish = nullptr;
    std::cout << "Ucitavam model goldfish..." << std::endl;
    try {
        goldfish = new Model("resources/models/goldfish/Goldfish_Low_Poly.obj");
        if (goldfish) {
            std::cout << "goldfish meshes = " << goldfish->meshes.size() << "\n";
            for (size_t i = 0; i < goldfish->meshes.size(); i++) {
                std::cout << "mesh[" << i << "] vertices=" << goldfish->meshes[i].vertices.size()
                    << " indices=" << goldfish->meshes[i].indices.size()
                    << " textures=" << goldfish->meshes[i].textures.size() << "\n";
            }
        }
        std::cout << "Model uspesno ucitan!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "GRESKA pri ucitavanju modela: " << e.what() << std::endl;
        goldfish = nullptr;
    }

    Model* seaweed1 = nullptr;
    std::cout << "Ucitavam model seaweed..." << std::endl;
    try {
        seaweed1 = new Model("resources/models/seaweed/seaweedList.obj");
        if (seaweed1) {
            std::cout << "seaweed meshes = " << seaweed1->meshes.size() << "\n";
            for (size_t i = 0; i < seaweed1->meshes.size(); i++) {
                std::cout << "mesh[" << i << "] vertices=" << seaweed1->meshes[i].vertices.size()
                    << " indices=" << seaweed1->meshes[i].indices.size()
                    << " textures=" << seaweed1->meshes[i].textures.size() << "\n";
            }
        }
        std::cout << "Model uspesno ucitan!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "GRESKA pri ucitavanju modela: " << e.what() << std::endl;
        seaweed1 = nullptr;
    }

    Model* seaweed2 = nullptr;
    std::cout << "Ucitavam model seaweed..." << std::endl;
    try {
        seaweed2 = new Model("resources/models/seaweed/seaweedList.obj");
        if (seaweed2) {
            std::cout << "seaweed meshes = " << seaweed2->meshes.size() << "\n";
            for (size_t i = 0; i < seaweed2->meshes.size(); i++) {
                std::cout << "mesh[" << i << "] vertices=" << seaweed2->meshes[i].vertices.size()
                    << " indices=" << seaweed2->meshes[i].indices.size()
                    << " textures=" << seaweed2->meshes[i].textures.size() << "\n";
            }
        }
        std::cout << "Model uspesno ucitan!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "GRESKA pri ucitavanju modela: " << e.what() << std::endl;
        seaweed2 = nullptr;
    }

    // Vertices -----------------------------------------------------------------------------------------------------------------------------------------------------

    // Aquarium frame
    const float* aquariumFrameVertices = getAquariumFrameVertices();
    size_t aquariumFrameFloatCount = getAquariumFrameFloatCount();

    const int floatsPerVertex = 10;
    int vertexCount = (int)(aquariumFrameFloatCount / floatsPerVertex);
    int quadCount = vertexCount / 4;

    // Aquarium glass
    const float* aquariumFrontGlassVertices = getAquariumFrontGlassVertices();
    const float* aquariumBackGlassVertices = getAquariumBackGlassVertices();
    const float* aquariumLeftGlassVertices = getAquariumLeftGlassVertices();
    const float* aquariumRightGlassVertices = getAquariumRightGlassVertices();

    // Chest
    chestBodyVertices = getChestBodyVertices();
    chestLidVertices = getChestLidVertices();

    glm::vec3 hingeA(0.0f), hingeB(0.0f);
    bool hingeReady = false;

    const int chestStride = CHEST_STRIDE_FLOATS;
    const int vcount = CHEST_VERTEX_COUNT;

    float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
    float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;

    for (int i = 0; i < vcount; i++) {
        float x = chestLidVertices[i * chestStride + 0];
        float y = chestLidVertices[i * chestStride + 1];
        float z = chestLidVertices[i * chestStride + 2];

        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
        minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
    }

    float hingeY = minY;
    float hingeZ = minZ;

    hingeA = glm::vec3(minX, hingeY, hingeZ);
    hingeB = glm::vec3(maxX, hingeY, hingeZ);

    hingeReady = true;

    // Sand
    Mesh sand = createSandGrid(80, 80, -1.65f, 1.65f, -0.75f, 0.75f, -0.85f, 6.0f);

    // Bubble
    Mesh bubble = createSphere(24, 48, 1.0f);

    // Signature
    float sigWpx = 220.0f;
    float sigHpx = 80.0f;
    float marginPx = 0.0f;

    auto pxToNdcX = [&](float px) { return 2.0f * (px / (float)screenWidth); };
    auto pxToNdcY = [&](float px) { return 2.0f * (px / (float)screenHeight); };

    float left = -1.0f + pxToNdcX(marginPx);
    float top = 1.0f - pxToNdcY(marginPx);
    float right = left + pxToNdcX(sigWpx);
    float bottom = top - pxToNdcY(sigHpx);

    float sigVerts[] = {
        left,  top,    0.0f,   0.0f, 1.0f,
        left,  bottom, 0.0f,   0.0f, 0.0f,
        right, bottom, 0.0f,   1.0f, 0.0f,

        left,  top,    0.0f,   0.0f, 1.0f,
        right, bottom, 0.0f,   1.0f, 0.0f,
        right, top,    0.0f,   1.0f, 1.0f
    };

    // VAOs & VBOs --------------------------------------------------------------------------------------------------------------------------------------------------

    // Aquarium
    unsigned int stride = (3 + 4 + 3) * sizeof(float);

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, aquariumFrameFloatCount * sizeof(float), aquariumFrameVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Chest
    glGenVertexArrays(1, &chestBodyVAO);
    glGenBuffers(1, &chestBodyVBO);

    glBindVertexArray(chestBodyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, chestBodyVBO);
    glBufferData(GL_ARRAY_BUFFER,
        CHEST_VERTEX_COUNT* CHEST_STRIDE_FLOATS * sizeof(float),
        chestBodyVertices,
        GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        CHEST_STRIDE_FLOATS * sizeof(float),
        (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        CHEST_STRIDE_FLOATS * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glGenVertexArrays(1, &chestLidVAO);
    glGenBuffers(1, &chestLidVBO);

    glBindVertexArray(chestLidVAO);
    glBindBuffer(GL_ARRAY_BUFFER, chestLidVBO);
    glBufferData(GL_ARRAY_BUFFER,
        CHEST_VERTEX_COUNT* CHEST_STRIDE_FLOATS * sizeof(float),
        chestLidVertices,
        GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        CHEST_STRIDE_FLOATS * sizeof(float),
        (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        CHEST_STRIDE_FLOATS * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Signature
    unsigned int sigVAO = 0, sigVBO = 0;

    glGenVertexArrays(1, &sigVAO);
    glGenBuffers(1, &sigVBO);

    glBindVertexArray(sigVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sigVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sigVerts), sigVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Shaderi ------------------------------------------------------------------------------------------------------------------------------------------------------

    Shader texturedShader("textured.vert", "textured.frag");
    Shader basicShader("basic.vert", "basic.frag");
    Shader modelShader("model.vert", "model.frag");
    Shader particleShader("particle.vert", "particle.frag");
    Shader transparentShader("transparent.vert", "transparent.frag");
    Shader entityShader("entity.vert", "entity.frag");

    // Matrica transformacija
    glm::mat4 model = glm::mat4(1.0f);

    // Matrica pogleda (kamere)
    glm::vec3 cameraPos = glm::vec3(0.0, 0.0, 2.0);
    glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 cameraTarget = glm::vec3(0.0, 0.0, 0.0);
    
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    // Matrica projekcije
    glm::mat4 projectionP = glm::perspective(glm::radians(90.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

    // Svijetla
    std::vector<PointLight> lights;
    lights.push_back({ glm::vec3(1.5f, 2.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.5f), 1.5f });
    lights.push_back({ glm::vec3(-2.0f, 1.0f, 2.0f), glm::vec3(0.05f, 0.7f, 1.0f), 1.5f });

    const std::vector<PointLight> baseLights = lights;

    // Render loop --------------------------------------------------------------------------------------------------------------------------------------------------
    
    // Boja pozadine
    glClearColor(0.5f, 0.6f, 0.8f, 1.0f);

    glCullFace(GL_BACK);

    while (!glfwWindowShouldClose(window))
    {
        double initFrameTime = glfwGetTime();

        static float lastT = (float)glfwGetTime();
        float t = (float)glfwGetTime();
        float dt = t - lastT;
        lastT = t;

        if (!processInput(window)) break;

        // Process fish hitbox
        float clownR = clownfishBaseRadius * (clownfishScale / clownfishBaseScale);
        float goldR = goldfishBaseRadius * (goldfishScale / goldfishBaseScale);

        // Process fish collision
        resolveSphereBounds(clownfishOffset, clownR);
        resolveSphereBounds(goldfishOffset, goldR);

        for (int it = 0; it < 3; it++) {
            resolveSphereSphere(clownfishOffset, clownR, goldfishOffset, goldR);
            resolveSphereBounds(clownfishOffset, clownR);
            resolveSphereBounds(goldfishOffset, goldR);
        }

        // Process fish rotation
        auto smoothExp = [&](const glm::vec3& cur, const glm::vec3& target, float k, float dt) {
            float a = 1.0f - expf(-k * dt);
            return glm::mix(cur, target, a);
        };

        glm::vec3 vc = (dt > 0.0f) ? (clownfishOffset - clownfishPrevPos) / dt : glm::vec3(0.0f);
        clownfishPrevPos = clownfishOffset;

        clownfishVelSmoothed = smoothExp(clownfishVelSmoothed, vc, ROT_FOLLOW_SPEED, dt);

        glm::vec3 dirc = glm::vec3(clownfishVelSmoothed.x, 0.0f, clownfishVelSmoothed.z);

        if (glm::length2(dirc) > MIN_SPEED * MIN_SPEED) {
            dirc = glm::normalize(dirc);

            clownfishForwardDyn = dirc;

            glm::quat targetQ = glm::rotation(glm::vec3(1, 0, 0), dirc);
            float a = 1.0f - expf(-ROT_FOLLOW_SPEED * dt);
            clownfishRot = glm::slerp(clownfishRot, targetQ, a);
        }

        glm::vec3 vg = (dt > 0.0f) ? (goldfishOffset - goldfishPrevPos) / dt : glm::vec3(0.0f);
        goldfishPrevPos = goldfishOffset;

        goldfishVelSmoothed = smoothExp(goldfishVelSmoothed, vg, ROT_FOLLOW_SPEED, dt);

        glm::vec3 dirg = glm::vec3(goldfishVelSmoothed.x, 0.0f, goldfishVelSmoothed.z);

        if (glm::length2(dirg) > MIN_SPEED * MIN_SPEED) {
            dirg = glm::normalize(dirg);

            goldfishForwardDyn = dirg;

            glm::quat targetQ = glm::rotation(glm::vec3(1, 0, 0), dirg);
            float a = 1.0f - expf(-ROT_FOLLOW_SPEED * dt);
            goldfishRot = glm::slerp(goldfishRot, targetQ, a);
        }

        // Process bubbles
        for (size_t i = 0; i < bubbles.size(); ) {
            bubbles[i].pos += bubbles[i].vel * dt;

            if (bubbles[i].pos.y > bubbles[i].killY) {
                bubbles[i] = bubbles.back();
                bubbles.pop_back();
                continue;
            }
            i++;
        }

        // Process food
        float foodStopY = -0.835f;

        for (size_t i = 0; i < foods.size(); )
        {
            auto& f = foods[i];

            if (!f.settled) {
                f.vel.y -= dt;

                f.pos += f.vel * dt;

                if (f.pos.y <= foodStopY) {
                    f.pos.y = foodStopY;
                    f.vel = glm::vec3(0.0f);
                    f.settled = true;
                }
            }

            if (f.pos.y < f.killY) {
                foods[i] = foods.back();
                foods.pop_back();
                continue;
            }

            i++;
        }

        for (size_t i = 0; i < foods.size(); )
        {
            bool eaten = false;

            if (hitSphere(clownfishOffset, clownR, foods[i].pos, foods[i].radius)) {
                clownfishScale *= 1.02f;
                eaten = true;
            }
            else if (hitSphere(goldfishOffset, goldR, foods[i].pos, foods[i].radius)) {
                goldfishScale *= 1.02f;
                eaten = true;
            }

            if (eaten) {
                foods[i] = foods.back();
                foods.pop_back();
                continue;
            }

            i++;
        }

        // Process chest
        if (chestLidAngle < chestLidTarget) {
            chestLidAngle += CHEST_LID_SPEED * dt;
            if (chestLidAngle > chestLidTarget)
                chestLidAngle = chestLidTarget;
        }
        else if (chestLidAngle > chestLidTarget) {
            chestLidAngle -= CHEST_LID_SPEED * dt;
            if (chestLidAngle < chestLidTarget)
                chestLidAngle = chestLidTarget;
        }
        if (chestLightValue < chestLightTarget) {
            chestLightValue += CHEST_LIGHT_FADE_SPEED * dt;
            if (chestLightValue > chestLightTarget) chestLightValue = chestLightTarget;
        }
        else if (chestLightValue > chestLightTarget) {
            chestLightValue -= CHEST_LIGHT_FADE_SPEED * dt;
            if (chestLightValue < chestLightTarget) chestLightValue = chestLightTarget;
        }

        //Testiranje dubine
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            glEnable(GL_DEPTH_TEST);
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            glDisable(GL_DEPTH_TEST);
        }

        //Odstranjivanje lica
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            glEnable(GL_CULL_FACE);
        }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            glDisable(GL_CULL_FACE);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update chest light
        lights = baseLights;

        glm::mat4 chestBaseTmp = glm::mat4(1.0f);
        chestBaseTmp = glm::translate(chestBaseTmp, glm::vec3(-0.7f, -0.7f, 0.3f));
        chestBaseTmp = glm::rotate(chestBaseTmp, glm::radians(125.0f), glm::vec3(0, 1, 0));
        chestBaseTmp = glm::scale(chestBaseTmp, glm::vec3(1.0f));

        if (chestLightValue > 0.001f) {
            glm::vec3 chestLightWorld = glm::vec3(chestBaseTmp * glm::vec4(chestLightLocal, 1.0f));
            float inten = chestLightIntensity * chestLightValue;
            lights.push_back({ chestLightWorld, chestLightColor, inten });
        }
        
        // Sand
        texturedShader.use();

        glm::mat4 sandM = glm::mat4(1.0f);
        texturedShader.setMat4("uM", sandM);
        texturedShader.setMat4("uV", view);
        texturedShader.setMat4("uP", projectionP);

        texturedShader.setVec3("uViewPos", cameraPos);
        uploadPointLights(texturedShader.getID(), lights);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sandTexture);
        texturedShader.setInt("uTexture", 0);

        glBindVertexArray(sand.VAO);
        glDrawElements(GL_TRIANGLES, sand.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);

        // Clownfish
        if (clownfish != nullptr) {
            entityShader.use();

            entityShader.setMat4("uV", view);
            entityShader.setMat4("uP", projectionP);
            uploadPointLights(entityShader.getID(), lights);
            entityShader.setVec3("uViewPos", cameraPos);
            entityShader.setVec3("uOffset", clownfishOffset);
            entityShader.setVec3("uVel", glm::vec3(0.0f));
            entityShader.setVec3("uAmp", glm::vec3(0.0f, 0.01f, 0.0f));
            entityShader.setFloat("uTime", (float)glfwGetTime());
            entityShader.setFloat("uFreq", 2.0f);

            glm::mat4 fishM(1.0f);

            glm::mat4 correction(1.0f);
            correction = glm::rotate(correction, glm::radians(90.0f), glm::vec3(0, 1, 0));

            glm::mat4 face = glm::mat4_cast(clownfishRot);

            fishM = face * correction;
            fishM = fishM * glm::scale(glm::mat4(1.0f), glm::vec3(clownfishScale));

            entityShader.setMat4("uM", fishM);

            clownfish->Draw(entityShader);
        }

        // Goldfish
        if (goldfish != nullptr) {
            entityShader.use();

            entityShader.setMat4("uV", view);
            entityShader.setMat4("uP", projectionP);
            uploadPointLights(entityShader.getID(), lights);
            entityShader.setVec3("uViewPos", cameraPos);
            entityShader.setVec3("uOffset", goldfishOffset);
            entityShader.setVec3("uVel", glm::vec3(0.0f));
            entityShader.setVec3("uAmp", glm::vec3(0.0f, 0.01f, 0.0f));
            entityShader.setFloat("uTime", (float)glfwGetTime());
            entityShader.setFloat("uFreq", 2.0f);

            glm::mat4 fishM(1.0f);

            glm::mat4 correction(1.0f);
            correction = glm::rotate(correction, glm::radians(90.0f), glm::vec3(0, 1, 0));

            glm::mat4 face = glm::mat4_cast(goldfishRot);

            fishM = face * correction;
            fishM = fishM * glm::scale(glm::mat4(1.0f), glm::vec3(goldfishScale));

            entityShader.setMat4("uM", fishM);

            goldfish->Draw(entityShader);
        }

        // Seaweed 1
        if (seaweed1 != nullptr) {
            modelShader.use();
            modelShader.setMat4("uV", view);
            modelShader.setMat4("uP", projectionP);
            modelShader.setVec3("uViewPos", cameraPos);
            uploadPointLights(modelShader.getID(), lights);

            glm::mat4 seaweed1Matrix = glm::mat4(1.0f);
            seaweed1Matrix = glm::translate(seaweed1Matrix, glm::vec3(0.1f, -0.8f, -0.2f));
            seaweed1Matrix = glm::rotate(seaweed1Matrix, glm::radians(-90.0f), glm::vec3(0, 1, 0));
            seaweed1Matrix = glm::scale(seaweed1Matrix, glm::vec3(0.15f));
            modelShader.setMat4("uM", seaweed1Matrix);

            seaweed1->Draw(modelShader);
        }

        // Seaweed 2
        if (seaweed2 != nullptr) {
            modelShader.use();
            modelShader.setMat4("uV", view);
            modelShader.setMat4("uP", projectionP);
            modelShader.setVec3("uViewPos", cameraPos);
            uploadPointLights(modelShader.getID(), lights);

            glm::mat4 seaweed2Matrix = glm::mat4(1.0f);
            seaweed2Matrix = glm::translate(seaweed2Matrix, glm::vec3(0.9f, -0.8f, 0.4f));
            seaweed2Matrix = glm::rotate(seaweed2Matrix, glm::radians(45.0f), glm::vec3(0, 1, 0));
            seaweed2Matrix = glm::scale(seaweed2Matrix, glm::vec3(0.15f));
            modelShader.setMat4("uM", seaweed2Matrix);

            seaweed2->Draw(modelShader);
        }

        // Chest
        texturedShader.use();

        texturedShader.setMat4("uV", view);
        texturedShader.setMat4("uP", projectionP);
        texturedShader.setVec3("uViewPos", cameraPos);
        uploadPointLights(texturedShader.getID(), lights);

        texturedShader.setInt("uTexture", 0);

        glm::mat4 chestBase = glm::mat4(1.0f);
        chestBase = glm::translate(chestBase, glm::vec3(-0.7f, -0.7f, 0.3f));
        chestBase = glm::rotate(chestBase, glm::radians(125.0f), glm::vec3(0, 1, 0));
        chestBase = glm::scale(chestBase, glm::vec3(1.0f));

        texturedShader.setMat4("uM", chestBase);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(chestBodyVAO);

        for (int face = 0; face < 6; face++) {
            glBindTexture(GL_TEXTURE_2D, chestBodyAtlasTex[face]);
            glDrawArrays(GL_TRIANGLES, face * 6, 6);
        }

        glBindVertexArray(0);

        glm::mat4 lidM = chestBase;

        if (hingeReady) {
            glm::vec3 axis = glm::normalize(hingeB - hingeA);

            lidM = lidM * glm::translate(glm::mat4(1.0f), hingeA);
            lidM = lidM * glm::rotate(glm::mat4(1.0f), glm::radians(-chestLidAngle), glm::vec3(1, 0, 0));
            lidM = lidM * glm::translate(glm::mat4(1.0f), -hingeA);
        }

        texturedShader.setMat4("uM", lidM);

        glBindVertexArray(chestLidVAO);

        for (int face = 0; face < 6; face++) {
            glBindTexture(GL_TEXTURE_2D, chestLidAtlasTex[face]);
            glDrawArrays(GL_TRIANGLES, face * 6, 6);
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Aquarium frame
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, aquariumFrameFloatCount * sizeof(float), aquariumFrameVertices, GL_DYNAMIC_DRAW);

        basicShader.use();

        glm::mat4 aquariumM = glm::mat4(1.0f);
        basicShader.setMat4("uM", aquariumM);
        basicShader.setMat4("uV", view);
        basicShader.setMat4("uP", projectionP);

        basicShader.setVec3("uViewPos", cameraPos);
        uploadPointLights(basicShader.getID(), lights);

        glBindVertexArray(VAO);

        for (int q = 0; q < quadCount; q++) {
            glDrawArrays(GL_TRIANGLE_FAN, q * 4, 4);
        }

        glBindVertexArray(0);

        // Aquarium glass
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        transparentShader.use();

        transparentShader.setMat4("uM", aquariumM);
        transparentShader.setMat4("uV", view);
        transparentShader.setMat4("uP", projectionP);
        transparentShader.setVec3("uViewPos", cameraPos);

        uploadPointLights(transparentShader.getID(), lights);

        transparentShader.setFloat("uAlphaMul", 1.0f);
        transparentShader.setFloat("uFresnelPow", 3.0f);
        transparentShader.setFloat("uRimBoost", 0.22f);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            getAquariumFrontGlassFloatCount() * sizeof(float),
            aquariumFrontGlassVertices,
            GL_STATIC_DRAW
        );
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, getAquariumFrontGlassFloatCount() / floatsPerVertex);

        glBufferData(
            GL_ARRAY_BUFFER,
            getAquariumBackGlassFloatCount() * sizeof(float),
            aquariumBackGlassVertices,
            GL_STATIC_DRAW
        );
        glDrawArrays(GL_TRIANGLE_FAN, 0, getAquariumBackGlassFloatCount() / floatsPerVertex);

        glBufferData(
            GL_ARRAY_BUFFER,
            getAquariumLeftGlassFloatCount() * sizeof(float),
            aquariumLeftGlassVertices,
            GL_STATIC_DRAW
        );
        glDrawArrays(GL_TRIANGLE_FAN, 0, getAquariumLeftGlassFloatCount() / floatsPerVertex);

        glBufferData(
            GL_ARRAY_BUFFER,
            getAquariumRightGlassFloatCount() * sizeof(float),
            aquariumRightGlassVertices,
            GL_STATIC_DRAW
        );
        glDrawArrays(GL_TRIANGLE_FAN, 0, getAquariumRightGlassFloatCount() / floatsPerVertex);

        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        // Bubbles
        if (!bubbles.empty()) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            particleShader.use();
            particleShader.setMat4("uV", view);
            particleShader.setMat4("uP", projectionP);
            particleShader.setVec3("uViewPos", cameraPos);

            glBindVertexArray(bubble.VAO);

            for (const auto& b : bubbles) {
                particleShader.setVec3("uCenter", b.pos);
                particleShader.setFloat("uRadius", b.radius);
                particleShader.setVec3("uColor", glm::vec3(0.3f, 0.8f, 1.0f));
                particleShader.setFloat("uAlpha", b.alpha);

                particleShader.setFloat("uTime", t + b.seed);
                particleShader.setVec3("uVel", glm::vec3(0.0f));
                particleShader.setVec3("uAmp", glm::vec3(0.01f, 0.0f, 0.01f));
                particleShader.setFloat("uFreq", 3.0f);

                glDrawElements(GL_TRIANGLES, bubble.indexCount, GL_UNSIGNED_INT, 0);
            }

            glBindVertexArray(0);
            glDepthMask(GL_TRUE);
        }

        // Food
        if (!foods.empty()) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            particleShader.use();
            particleShader.setMat4("uV", view);
            particleShader.setMat4("uP", projectionP);
            particleShader.setVec3("uViewPos", cameraPos);

            glBindVertexArray(bubble.VAO);

            for (const auto& f : foods) {
                particleShader.setVec3("uCenter", f.pos);
                particleShader.setFloat("uRadius", f.radius);
                particleShader.setVec3("uColor", glm::vec3(0.4f, 0.2f, 0.05f));
                particleShader.setFloat("uAlpha", 0.95f);
                particleShader.setFloat("uTime", t);
                particleShader.setVec3("uVel", glm::vec3(0.0f));
                particleShader.setVec3("uAmp", glm::vec3(0.0f));
                particleShader.setFloat("uFreq", 0.0f);

                glDrawElements(GL_TRIANGLES, bubble.indexCount, GL_UNSIGNED_INT, 0);
            }

            glBindVertexArray(0);
            glDepthMask(GL_TRUE);
        }

        // Signature
        texturedShader.use();

        glm::mat4 I(1.0f);
        texturedShader.setMat4("uM", I);
        texturedShader.setMat4("uV", I);
        texturedShader.setMat4("uP", I);
        texturedShader.setVec3("uViewPos", cameraPos);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, signatureTexture);

        texturedShader.setInt("uTexture", 0);

        glBindVertexArray(sigVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);

        // Buffer swap
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Frame limiter
        while (glfwGetTime() - initFrameTime < 1. / FRAME_LIMIT) {}
    }

    // Cleanup ------------------------------------------------------------------------------------------------------------------------------------------------------

    if (clownfish) { delete clownfish; clownfish = nullptr; }
    if (goldfish) { delete goldfish; goldfish = nullptr; }
    if (seaweed1) { delete seaweed1; seaweed1 = nullptr; }
    if (seaweed2) { delete seaweed2; seaweed2 = nullptr; }

    glDeleteTextures(1, &sandTexture);
    glDeleteTextures(1, &signatureTexture);

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &sand.VBO);
    glDeleteBuffers(1, &sand.EBO);
    glDeleteBuffers(1, &bubble.VBO);
    glDeleteBuffers(1, &bubble.EBO);
    glDeleteBuffers(1, &chestBodyVBO);
    glDeleteBuffers(1, &chestLidVBO);
    glDeleteBuffers(1, &sigVBO);
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &sand.VAO);
    glDeleteVertexArrays(1, &bubble.VAO);
    glDeleteVertexArrays(1, &chestBodyVAO);
    glDeleteVertexArrays(1, &chestLidVAO);
    glDeleteVertexArrays(1, &sigVAO);

    glDeleteTextures(6, chestBodyAtlasTex);
    glDeleteTextures(6, chestLidAtlasTex);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
