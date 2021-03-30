//Application class
#pragma once

#include "Camera.h"
#include "Texture.h"

#include <memory>
#include <GLFW/glfw3.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

struct AppState {
public:
    float deltaTime = 0.0f; //Time between current frame and last frame
    float lastFrame = 0.0f; //Time of last frame
    float lastX = 1000, lastY = 500; //Last cursor position
    bool firstMouse = true; //true if mouse didn't move
    int filling = 0; //каркасный режим или нет
    std::vector<bool> keys; //массив состояний кнопок - нажата/не нажата
    bool g_captureMouse = true; //Мышка захвачена нашим приложением или нет?
    Camera camera; //camera

    AppState() : keys(1024, 0), camera(glm::vec3(0., 0., 3.0)) {}
};

class App {
public:
    App(const std::string &pathToConfig) {
        std::ifstream input(pathToConfig);
        input >> config;
    }

    App(const App&) = delete;

    App& operator=(const App& other) = delete;

    int Run();

private:
    //TODO: reimplement this
    //Possible option:
    //1. Define Keybord, Mouse etc classes with parts of this state and corresponding callbacks
    //2. Unify them into one class with getters
    //3. Put instance of this class here
    AppState state; //we need to separate this into struct because of C-style GLFW callbacks

    std::vector<std::unique_ptr<Texture>> textures; // textures and mesh
    GLuint g_vertexBufferObject;
    GLuint g_vertexArrayObject;

    nlohmann::json config; //application config
    GLFWwindow* window; //window

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