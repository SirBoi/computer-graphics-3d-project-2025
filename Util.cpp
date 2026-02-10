#include "Util.h";

#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <sstream>
#include <iostream>

#include "stb_image.h"
#include <vector>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>


Mesh createSandGrid(int nx, int nz, float x0, float x1, float z0, float z1, float y, float uvTile) {
    std::vector<float> v;
    std::vector<unsigned int> idx;

    v.reserve((nx + 1) * (nz + 1) * 8);
    idx.reserve(nx * nz * 6);

    for (int j = 0; j <= nz; ++j) {
        float tj = (float)j / nz;
        float z = z0 + (z1 - z0) * tj;

        for (int i = 0; i <= nx; ++i) {
            float ti = (float)i / nx;
            float x = x0 + (x1 - x0) * ti;

            v.push_back(x);
            v.push_back(y);
            v.push_back(z);

            v.push_back(ti * uvTile);
            v.push_back(tj * uvTile);

            v.push_back(0.0f);
            v.push_back(1.0f);
            v.push_back(0.0f);
        }
    }

    auto at = [nx](int i, int j) {
        return (unsigned int)(j * (nx + 1) + i);
        };

    for (int j = 0; j < nz; ++j) {
        for (int i = 0; i < nx; ++i) {
            unsigned int i0 = at(i, j);
            unsigned int i1 = at(i + 1, j);
            unsigned int i2 = at(i + 1, j + 1);
            unsigned int i3 = at(i, j + 1);

            idx.push_back(i0); idx.push_back(i1); idx.push_back(i2);
            idx.push_back(i0); idx.push_back(i2); idx.push_back(i3);
        }
    }

    Mesh m;

    glGenVertexArrays(1, &m.VAO);
    glBindVertexArray(m.VAO);

    glGenBuffers(1, &m.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

    GLsizei stride = 8 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m.indexCount = (GLsizei)idx.size();
    return m;
}

Mesh createSphere(int stacks, int slices, float radius) {
    Mesh m;

    stacks = std::max(2, stacks);
    slices = std::max(3, slices);

    std::vector<float> v;
    std::vector<unsigned int> idx;

    v.reserve((stacks + 1) * (slices + 1) * 8);
    idx.reserve(stacks * slices * 6);

    const float PI = 3.14159265358979323846f;

    for (int i = 0; i <= stacks; i++) {
        float t = (float)i / (float)stacks;
        float phi = t * PI;

        float y = std::cos(phi);
        float r = std::sin(phi);

        for (int j = 0; j <= slices; j++) {
            float s = (float)j / (float)slices;
            float theta = s * 2.0f * PI;

            float x = r * std::cos(theta);
            float z = r * std::sin(theta);

            v.push_back(x * radius);
            v.push_back(y * radius);
            v.push_back(z * radius);

            v.push_back(x);
            v.push_back(y);
            v.push_back(z);

            v.push_back(s);
            v.push_back(1.0f - t);
        }
    }

    auto at = [slices](int i, int j) {
        return (unsigned int)(i * (slices + 1) + j);
        };

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            unsigned int i0 = at(i, j);
            unsigned int i1 = at(i + 1, j);
            unsigned int i2 = at(i + 1, j + 1);
            unsigned int i3 = at(i, j + 1);

            idx.push_back(i0); idx.push_back(i1); idx.push_back(i2);
            idx.push_back(i0); idx.push_back(i2); idx.push_back(i3);
        }
    }

    glGenVertexArrays(1, &m.VAO);
    glBindVertexArray(m.VAO);

    glGenBuffers(1, &m.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

    GLsizei stride = 8 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m.indexCount = (GLsizei)idx.size();
    return m;
}


static inline void useProgram(GLuint program) {
    glUseProgram(program);
}

void uploadViewPos(GLuint program, const glm::vec3& viewPos) {
    useProgram(program);
    GLint loc = glGetUniformLocation(program, "uViewPos");
    if (loc != -1) glUniform3fv(loc, 1, glm::value_ptr(viewPos));
}

void uploadPointLights(GLuint program, const std::vector<PointLight>& lights) {
    useProgram(program);

    int count = (int)std::min<size_t>(lights.size(), 8);

    GLint countLoc = glGetUniformLocation(program, "uLightCount");
    if (countLoc != -1) glUniform1i(countLoc, count);

    for (int i = 0; i < count; i++) {
        std::string idx = "[" + std::to_string(i) + "]";

        GLint posLoc = glGetUniformLocation(program, ("uLightPos" + idx).c_str());
        GLint colLoc = glGetUniformLocation(program, ("uLightColor" + idx).c_str());
        GLint intLoc = glGetUniformLocation(program, ("uLightIntensity" + idx).c_str());

        if (posLoc != -1) glUniform3fv(posLoc, 1, &lights[i].pos[0]);
        if (colLoc != -1) glUniform3fv(colLoc, 1, &lights[i].color[0]);
        if (intLoc != -1) glUniform1f(intLoc, lights[i].intensity);
    }
}

int endProgram(std::string message) {
    glfwTerminate();
    return -1;
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;

    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
    }
    else {
        ss << "";
    }

    std::string temp = ss.str();
    const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)

    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); //Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); //Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth = 0;
    int TextureHeight = 0;
    int TextureChannels = 0;

    // Flip while LOADING (official stb API)
    stbi_set_flip_vertically_on_load(true);

    unsigned char* ImageData =
        stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, STBI_rgb_alpha);

    if (!ImageData) {
        return 0;
    }

    // Because we forced STBI_rgb_alpha:
    TextureChannels = 4;
    GLint InternalFormat = GL_RGBA;
    GLenum DataFormat = GL_RGBA;

    unsigned int Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);

    // Safe for any width
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat,
        TextureWidth, TextureHeight,
        0, DataFormat, GL_UNSIGNED_BYTE, ImageData);

    glGenerateMipmap(GL_TEXTURE_2D);

    // Basic sampling params (you can tweak later)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(ImageData);
    return Texture;
}

GLFWcursor* loadImageToCursor(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);

    if (ImageData != NULL)
    {
        GLFWimage image;
        image.width = TextureWidth;
        image.height = TextureHeight;
        image.pixels = ImageData;

        // Tacka na površini slike kursora koja se ponaša kao hitboks
        int hotspotX = image.width / 6;
        int hotspotY = image.height / 6;

        GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
        stbi_image_free(ImageData);
        return cursor;
    }
    else {
        stbi_image_free(ImageData);
    }
}

bool hitSphere(const glm::vec3& a, float ar, const glm::vec3& b, float br)
{
    float rr = ar + br;
    return glm::dot(a - b, a - b) <= rr * rr;
}
