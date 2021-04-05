//Application class
#pragma once

#include "Camera.h"
#include "Models/Texture.h"
#include "Models/Mesh.h"
#include "Models/Material.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

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
    bool visualizeNormalsWithColor = false; //normals visualization (with color)
    bool edgeDetection = false; //edgge detection after rendering
    bool visDepthMap = false; //visualize depth map
    Camera camera; //camera

    AppState()
        : keys(1024, 0)
        , camera(glm::vec3(-125.0f, 150.0f, 0.0f))
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
        {0, Material()}
    }; //add default material
    std::vector<std::unique_ptr<Mesh>> scene;

    nlohmann::json config; //application config
    GLFWwindow* window; //window
    float printEvery = 1.0f;

    //color buffer
    GLuint colorBufferFBO;
    GLuint colorBufferRBO;
    GLuint colorBufferTexture;

    //depth map
    GLuint depthMapFBO;
    GLuint depthMapTexture;
    const uint32_t depthMapWidth = 4096;
    const uint32_t depthMapHeight = 4096;

    //simple quad that fills screen
    GLuint quadVAO;
    GLuint quadVBO;
    GLuint quadEBO;

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
    void release();
};