#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>

#include "stb_image.h"
#include "shader.h"
#include "camera.h"
#include "perlin.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float VERTEX_COUNT = 500;


Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

bool firstMouse = true;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLFWwindow *window = nullptr;

// Map params
float WATER_HEIGHT = 0.1;

// Noise params
int octaves = 5;
float meshHeight = 32;
float noiseScale = 64;
float persistence = 0.5;
float lacunarity = 2;

void initGlfw();

void framebufferSizeCallback(GLFWwindow *, int width, int height);

void processInput(GLFWwindow *window);

void mouseCallback(GLFWwindow *window, double xpos, double ypos);

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

unsigned int loadTexture(const char *path);

unsigned int loadCubemap(std::vector<std::string> faces);

void createCube(unsigned int &VAO, unsigned int &VBO);

void createSkybox(unsigned int &texture, unsigned int &VAO, unsigned int &VBO);

void lightningShaderSettings(Shader &lightingShader);

std::vector<int> generate_indices();

std::vector<float> generate_noise_map(int xOffset, int yOffset);

std::vector<float> generate_vertices(const std::vector<float> &noise_map);

std::vector<float> generate_normals(const std::vector<int> &indices, const std::vector<float> &vertices);

std::vector<float> generate_biome(const std::vector<float> &vertices, int xOffset, int yOffset);

void generate_map_chunk(GLuint &VAO, std::vector<int> &indices, int xOffset, int yOffset);

int main() {
    initGlfw();
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "RAF RG Projekat", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

//    SETUP END

//    TEXTURES
    unsigned int diffuseMap = loadTexture("../../resources/textures/container2.png");
    unsigned int specularMap = loadTexture("../../resources/textures/container2_specular.png");
    unsigned int grassTexture = loadTexture("../../resources/textures/grass.png");
    unsigned int rockTexture = loadTexture("../../resources/textures/mountains.jpg");
    unsigned int heightMap = loadTexture("../../resources/heightmap.bmp");

    // TERRAIN
    unsigned int terrainVAO;
    std::vector<int> terrainIndices;
    generate_map_chunk(terrainVAO, terrainIndices, 0, 0);


    // CUBE
    unsigned int cubeVAO, cubeVBO;
    createCube(cubeVAO, cubeVBO);

    // SKYBOX
    unsigned int skyboxTexture, skyboxVAO, skyboxVBO;
    createSkybox(skyboxTexture, skyboxVAO, skyboxVBO);
    Shader skyboxShader(
            "../../resources/shaders/skybox.vert",
            "../../resources/shaders/skybox.frag"
    );
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

//    LIGHT
    Shader lightShader(
            "../../resources/shaders/light.vert",
            "../../resources/shaders/light.frag"
    );
    lightShader.use();
    lightShader.setInt("material.diffuse", 0);
    lightShader.setInt("material.specular", 1);

//    RENDER LOOP
    while (!glfwWindowShouldClose(window)) {
        // DELTA TIME
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // INPUT
        processInput(window);

        // RENDER
//        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // LIGHT
        lightShader.use();
        lightningShaderSettings(lightShader);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f,
                                                100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // world transformation
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);

//        TERRAIN
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(
                -VERTEX_COUNT / 2.0 + (VERTEX_COUNT - 1) * 0,
                -15.0,
                -VERTEX_COUNT / 2.0 + (VERTEX_COUNT - 1) * 0
                               )
        );

        lightShader.setMat4("model", model);

//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, nullptr);

        // CUBE
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, diffuseMap);
//        glActiveTexture(GL_TEXTURE1);
//        glBindTexture(GL_TEXTURE_2D, specularMap);
//        glBindVertexArray(cubeVAO);
//        glDrawArrays(GL_TRIANGLES, 0, 36);

        // SKYBOX
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        // END
        glfwSwapBuffers(window);
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &skyboxVBO);

    glfwTerminate();
    return 0;
}

void framebufferSizeCallback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
}

void initGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const *path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        } else {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


void createCube(unsigned int &VAO, unsigned int &VBO) {
    float cubeVertices[] = {
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}


void createSkybox(unsigned int &texture, unsigned int &VAO, unsigned int &VBO) {
    std::vector<std::string> faces{
            "../../resources/skybox/right.jpg",
            "../../resources/skybox/left.jpg",
            "../../resources/skybox/top.jpg",
            "../../resources/skybox/bottom.jpg",
            "../../resources/skybox/front.jpg",
            "../../resources/skybox/back.jpg"
    };

    texture = loadCubemap(faces);

    float skyboxVertices[] = {
            // positions
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
}

void lightningShaderSettings(Shader &lightingShader) {
    glm::vec3 pointLightPositions[] = {
            glm::vec3(0.7f, 0.2f, 2.0f),
            glm::vec3(2.3f, -3.3f, -4.0f),
            glm::vec3(-4.0f, 2.0f, -12.0f),
            glm::vec3(0.0f, 0.0f, -3.0f)
    };

    lightingShader.setVec3("viewPos", camera.Position);
    lightingShader.setFloat("material.shininess", 32.0f);        // light properties
    // directional light
    lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
    lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
    lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
    // point light 1
    lightingShader.setVec3("pointLights[0].position", pointLightPositions[0]);
    lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
    lightingShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("pointLights[0].constant", 1.0f);
    lightingShader.setFloat("pointLights[0].linear", 0.09);
    lightingShader.setFloat("pointLights[0].quadratic", 0.032);
    // point light 2
    lightingShader.setVec3("pointLights[1].position", pointLightPositions[1]);
    lightingShader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
    lightingShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("pointLights[1].constant", 1.0f);
    lightingShader.setFloat("pointLights[1].linear", 0.09);
    lightingShader.setFloat("pointLights[1].quadratic", 0.032);
    // point light 3
    lightingShader.setVec3("pointLights[2].position", pointLightPositions[2]);
    lightingShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
    lightingShader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("pointLights[2].constant", 1.0f);
    lightingShader.setFloat("pointLights[2].linear", 0.09);
    lightingShader.setFloat("pointLights[2].quadratic", 0.032);
    // point light 4
    lightingShader.setVec3("pointLights[3].position", pointLightPositions[3]);
    lightingShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
    lightingShader.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
    lightingShader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("pointLights[3].constant", 1.0f);
    lightingShader.setFloat("pointLights[3].linear", 0.09);
    lightingShader.setFloat("pointLights[3].quadratic", 0.032);
    // spotLight
    lightingShader.setVec3("spotLight.position", camera.Position);
    lightingShader.setVec3("spotLight.direction", camera.Front);
    lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    lightingShader.setFloat("spotLight.constant", 1.0f);
    lightingShader.setFloat("spotLight.linear", 0.09);
    lightingShader.setFloat("spotLight.quadratic", 0.032);
    lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(0.0)));
    lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(0.0f)));
}

std::vector<int> generate_indices() {
    std::vector<int> indices;

    for (int y = 0; y < VERTEX_COUNT; y++)
        for (int x = 0; x < VERTEX_COUNT; x++) {
            int pos = x + y * VERTEX_COUNT;

            if (x == VERTEX_COUNT - 1 || y == VERTEX_COUNT - 1) {
                // Don't create indices for right or top edge
                continue;
            } else {
                // Top left triangle of square
                indices.push_back(pos + VERTEX_COUNT);
                indices.push_back(pos);
                indices.push_back(pos + VERTEX_COUNT + 1);
                // Bottom right triangle of square
                indices.push_back(pos + 1);
                indices.push_back(pos + 1 + VERTEX_COUNT);
                indices.push_back(pos);
            }
        }

    return indices;
}

float randomModifier() {
    float modifier = ((float) rand()) / RAND_MAX;
    srand((unsigned int) time(NULL));
    modifier = ((float) rand()) / RAND_MAX;
    modifier = ((float) rand()) / RAND_MAX;
    return modifier + 1.0f;
}

std::vector<float> generate_noise_map(int offsetX, int offsetY) {
    std::vector<float> noiseValues;
    std::vector<float> normalizedNoiseValues;
    std::vector<int> p = get_permutation_vector();

    float amp = 1;
    float freq = 1;
    float maxPossibleHeight = 0;

    for (int i = 0; i < octaves; i++) {
        maxPossibleHeight += amp;
        amp *= persistence;
    }

    for (int y = 0; y < VERTEX_COUNT; y++) {

        for (int x = 0; x < VERTEX_COUNT; x++) {
            float modifier = randomModifier();
            amp = 1;
            freq = 1;
            float noiseHeight = 0;
            for (int i = 0; i < octaves; i++) {
                float xSample = (x + offsetX * (VERTEX_COUNT - 1)) / noiseScale * freq;
                float ySample = (y + offsetY * (VERTEX_COUNT - 1)) / noiseScale * freq;

                float perlinValue = perlin_noise(xSample * modifier, ySample * modifier, p);
                noiseHeight += perlinValue * amp;
                amp *= persistence;
                freq *= lacunarity;
            }

            noiseValues.push_back(noiseHeight);
        }
    }

    for (int y = 0; y < VERTEX_COUNT; y++) {
        for (int x = 0; x < VERTEX_COUNT; x++) {
            // Inverse lerp and scale values to range from 0 to 1
            normalizedNoiseValues.push_back((noiseValues[x + y * VERTEX_COUNT] + 1) / maxPossibleHeight);
        }
    }

    return normalizedNoiseValues;
}

std::vector<float> generate_vertices(const std::vector<float> &noise_map) {
    std::vector<float> v;

    for (int y = 0; y < VERTEX_COUNT + 1; y++)
        for (int x = 0; x < VERTEX_COUNT; x++) {
            v.push_back(x);
            // Apply cubic easing to the noise
            float easedNoise = std::pow(noise_map[x + y * VERTEX_COUNT] * 1.1, 3);
            // Scale noise to match meshHeight
            // Pervent vertex height from being below WATER_HEIGHT
            v.push_back(std::fmax(easedNoise * meshHeight, WATER_HEIGHT * 0.5 * meshHeight));
            v.push_back(y);
        }

    return v;
}

std::vector<float> generate_normals(const std::vector<int> &indices, const std::vector<float> &vertices) {
    int pos;
    glm::vec3 normal;
    std::vector<float> normals;
    std::vector<glm::vec3> verts;

    // Get the vertices of each triangle in mesh
    // For each group of indices
    for (int i = 0; i < indices.size(); i += 3) {

        // Get the vertices (point) for each index
        for (int j = 0; j < 3; j++) {
            pos = indices[i + j] * 3;
            verts.push_back(glm::vec3(vertices[pos], vertices[pos + 1], vertices[pos + 2]));
        }

        // Get vectors of two edges of triangle
        glm::vec3 U = verts[i + 1] - verts[i];
        glm::vec3 V = verts[i + 2] - verts[i];

        // Calculate normal
        normal = glm::normalize(-glm::cross(U, V));
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
    }

    return normals;
}

glm::vec3 get_color(int r, int g, int b) {
    return glm::vec3(r / 255.0, g / 255.0, b / 255.0);
}


struct terrainColor {
    terrainColor(float _height, glm::vec3 _color) {
        height = _height;
        color = _color;
    };
    float height;
    glm::vec3 color;
};

std::vector<float> generate_biome(const std::vector<float> &vertices, int xOffset, int yOffset) {
    std::vector<float> colors;
    std::vector<terrainColor> biomeColors;
    glm::vec3 color = get_color(255, 255, 255);

    // NOTE: Terrain color height is a value between 0 and 1
    biomeColors.push_back(terrainColor(WATER_HEIGHT * 0.5, get_color(60, 95, 190)));   // Deep water
    biomeColors.push_back(terrainColor(WATER_HEIGHT, get_color(60, 100, 190)));  // Shallow water
    biomeColors.push_back(terrainColor(0.15, get_color(210, 215, 130)));                // Sand
    biomeColors.push_back(terrainColor(0.30, get_color(95, 165, 30)));                // Grass 1
    biomeColors.push_back(terrainColor(0.40, get_color(65, 115, 20)));                // Grass 2
    biomeColors.push_back(terrainColor(0.50, get_color(90, 65, 60)));                // Rock 1
    biomeColors.push_back(terrainColor(0.80, get_color(75, 60, 55)));                // Rock 2
    biomeColors.push_back(terrainColor(1.00, get_color(255, 255, 255)));                // Snow

    std::string plantType;

    for (int i = 1; i < vertices.size(); i += 3) {
        colors.push_back(color.r);
        colors.push_back(color.g);
        colors.push_back(color.b);
    }
    return colors;
}

void generate_map_chunk(unsigned int &VAO, std::vector<int> &indices, int xOffset, int yOffset) {
    std::vector<float> noise_map;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;

    // Generate map
    indices = generate_indices();
    noise_map = generate_noise_map(xOffset, yOffset);
    vertices = generate_vertices(noise_map);
    normals = generate_normals(indices, vertices);
    colors = generate_biome(vertices, xOffset, yOffset);

    unsigned int pVBO, nVBO, cVBO, EBO;

    // Create buffers and arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &pVBO);
    glGenBuffers(1, &nVBO);
    glGenBuffers(1, &cVBO);
    glGenBuffers(1, &EBO);

    // Bind vertices to VBO
    glBindVertexArray(VAO);

    // Create element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);

    // position
    glBindBuffer(GL_ARRAY_BUFFER, pVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    // normals
    glBindBuffer(GL_ARRAY_BUFFER, nVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

//    // Bind vertices to VBO
    glBindBuffer(GL_ARRAY_BUFFER, cVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), &colors[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}
