//Application class
#pragma once

#include "Camera.h"
#include "Models/Material.h"
#include "Models/Mesh.h"
#include "Models/Texture.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

enum RenderingMode {
    DEFAULT = 0,
    SHADOW_MAP,
    NORMALS_COLOR,
    EDGE_DETECTION,
};

struct AppState {
public:
    float deltaTime = 0.0f; //Time between current frame and last frame
    float lastFrame = 0.0f; //Time of last frame
    float lastX = 1000, lastY = 500; //Last cursor position
    bool firstMouse = true; //true if mouse didn't move
    int filling = 1; //каркасный режим или нет
    std::vector<bool> keys; //массив состояний кнопок - нажата/не нажата
    bool g_captureMouse = true; //Мышка захвачена нашим приложением или нет?
    bool isFlashlightOn = false; //Is flashlight on?
    RenderingMode renderingMode = RenderingMode::DEFAULT;
    Camera camera; //camera

    //TODO: move this to config
    AppState()
        : keys(1024, 0)
        , camera(glm::vec3(-1057.16f, 578.956f, 91.7585f), glm::vec3(0.0f, 1.0f, 0.0f), -6.68971f, 6.03009f)
    {
    }
};

class App {
public:
    App(const std::string& pathToConfig);

    App(const App&) = delete;

    App& operator=(const App& other) = delete;

    int Run();

private:
    //TODO: reimplement this
    //Possible solution:
    //1. Define Keybord, Mouse etc classes with parts of this state and corresponding callbacks
    //2. Unify them into one class with getters
    //3. Put instance of this class here
    AppState state; //we need to separate this into struct because of C-style GLFW callbacks

    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
    std::unordered_map<uint32_t, Material> materials = {
        { 0, Material() }
    }; //add default material
    //have to use shared_ptr for cloth simulation to work
    //(need a pointer to the same Mesh here and in Cloth)
    std::vector<std::shared_ptr<Mesh>> scene;
    std::uint32_t lightIdx = 0;

    std::vector<std::vector<std::size_t>> sideSplit; //first - twosided, second - onesided
    std::unordered_map<std::uint32_t, std::vector<glm::mat4>> duplicatedModels;

    nlohmann::json config; //application config
    GLFWwindow* window; //window
    float printEvery = 1.0f;
    std::string shadersPath;

    //color buffer
    GLuint colorBufferFBO;
    GLuint colorBufferRBO;
    GLuint colorBufferTexture;
    void setupColorBuffer();
    void deleteColorBuffer();
    void renderScene(ShaderProgram& lightningProgram, ShaderProgram& sourceProgram);

    //shadow map
    //TODO: move this to separate class
    GLuint shadowMapFBO;
    GLuint shadowMapRBO;
    std::vector<GLuint> shadowMapTextures;
    const uint32_t shadowMapWidth = 4096;
    const uint32_t shadowMapHeight = 4096;
    glm::vec3 lightDir;
    glm::mat4 lightSpaceMatrix;
    void setupShadowMapBuffer();
    void deleteShadowMapBuffer();
    void renderShadowMap(ShaderProgram& depthProgram, ShaderProgram& quadDepthProgram);

    //point shadow map
    //TODO: move this to separate class
    GLuint pointShadowMapFBO;
    GLuint pointShadowMapRBO;
    GLuint pointShadowMapTexture;
    const uint32_t pointShadowMapWidth = 1024;
    const uint32_t pointShadowMapHeight = 1024;
    glm::vec3 lightPos;
    float nearPlane;
    float farPlane;
    std::vector<glm::mat4> lightSpaceTransforms;
    void setupPointShadowMapBuffer();
    void deletePointShadowMapBuffer();
    void renderPointShadowMap(ShaderProgram& depthProgram);

    //simple quad that fills screen
    //TODO: move this to Mesh.cpp
    GLuint quadVAO;
    GLuint quadVBO;
    GLuint quadEBO;
    void setupQuad();
    void deleteQuad();
    void visualizeShadowMap(ShaderProgram& quadDepthProgram);
    void visualizeScene(ShaderProgram& quadColorProgram);

    int initGL() const;

    // callbacks
    static void OnKeyboardPressed(GLFWwindow* window, int key, int /* scancode */, int action, int /* mode */);
    static void OnMouseButtonClicked(GLFWwindow* window, int button, int action, int /* mods */);
    static void OnMouseMove(GLFWwindow* window, double xpos, double ypos);
    static void OnMouseScroll(GLFWwindow* window, double /* xoffset */, double yoffset);

    //move camera
    void doCameraMovement();

    //create GLFW window
    int createWindow();

    //load geometry
    void loadModels();

    //main application loop
    void mainLoop();

    //release GPU resources
    //TODO: move this to destructor
    void release();
};