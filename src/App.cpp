#include "App.h"
#include "Models/ImportScene.h"
#include "ShaderProgram.h"

App::App(const std::string& pathToConfig)
{
    std::ifstream input(pathToConfig);
    input >> config;

    state.camera.MouseSensitivity = config["MouseSensitivity"];
    state.lastX = static_cast<float>(config["width"]) / 2.0f;
    state.lastY = static_cast<float>(config["height"]) / 2.0f;
}

int App::initGL() const
{
    //грузим функции opengl через glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return 0;
}

int App::createWindow()
{
    if (!glfwInit()) {
        return -1;
    }

    //запрашиваем контекст opengl версии 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(config["width"], config["height"], std::string(config["name"]).c_str(), nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (initGL() != 0) {
        return -1;
    }

    //Set pointer to state so we can get it from callbacks
    glfwSetWindowUserPointer(window, &state);

    //Reset any OpenGL errors which could be present for some reason
    GLenum gl_error = glGetError();
    while (gl_error != GL_NO_ERROR) {
        gl_error = glGetError();
    }
    return 0;
}

void App::OnKeyboardPressed(GLFWwindow* window, int key, int /* scancode */, int action, int /* mode */)
{
    AppState* state = reinterpret_cast<AppState*>(glfwGetWindowUserPointer(window));

    switch (key) {
    case GLFW_KEY_ESCAPE: //на Esc выходим из программы
        if (action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        break;
    case GLFW_KEY_SPACE: //на пробел переключение в каркасный режим и обратно
        if (action == GLFW_PRESS) {
            if (state->filling == 0) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                state->filling = 1;
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                state->filling = 0;
            }
        }
        break;
    case GLFW_KEY_T: //turn flashlight on/off
        if (action == GLFW_PRESS) {
            state->isFlashlightOn = !(state->isFlashlightOn);
        }
        break;
    default:
        if (action == GLFW_PRESS) {
            (state->keys)[key] = true;
        } else if (action == GLFW_RELEASE) {
            (state->keys)[key] = false;
        }
    }
}

void App::OnMouseButtonClicked(GLFWwindow* window, int button, int action, int /* mods */)
{
    AppState* state = reinterpret_cast<AppState*>(glfwGetWindowUserPointer(window));

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        state->g_captureMouse = !(state->g_captureMouse);
    }

    if (state->g_captureMouse) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void App::OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
    AppState* state = reinterpret_cast<AppState*>(glfwGetWindowUserPointer(window));

    if (state->firstMouse) {
        state->lastX = float(xpos);
        state->lastY = float(ypos);
        state->firstMouse = false;
    }

    GLfloat xoffset = float(xpos) - state->lastX;
    GLfloat yoffset = state->lastY - float(ypos);

    state->lastX = float(xpos);
    state->lastY = float(ypos);

    if (state->g_captureMouse) {
        (state->camera).ProcessMouseMovement(xoffset, yoffset);
    }
}

void App::OnMouseScroll(GLFWwindow* window, double /* xoffset */, double yoffset)
{
    AppState* state = reinterpret_cast<AppState*>(glfwGetWindowUserPointer(window));

    (state->camera).ProcessMouseScroll(GLfloat(yoffset));
}

void App::doCameraMovement()
{
    if ((state.keys)[GLFW_KEY_W]) {
        state.camera.ProcessKeyboard(FORWARD, state.deltaTime);
    }
    if ((state.keys)[GLFW_KEY_A]) {
        state.camera.ProcessKeyboard(LEFT, state.deltaTime);
    }
    if ((state.keys)[GLFW_KEY_S]) {
        state.camera.ProcessKeyboard(BACKWARD, state.deltaTime);
    }
    if ((state.keys)[GLFW_KEY_D]) {
        state.camera.ProcessKeyboard(RIGHT, state.deltaTime);
    }
}

void App::loadModels()
{
    //load textures
    std::vector<std::string> names = {
        "container2.png",
        "container2_specular.png",
    };
    for (const std::string& name : names) {
        std::string path = std::string(config["dataPath"]) + "/textures/" + name;
        textures[path] = std::make_unique<Texture>(path);
        textures[path]->GLLoad();
        GL_CHECK_ERRORS;
    }

    //create meshes
    importSceneFromFile(
        std::string(config["dataPath"]) + "/bunny/reconstruction/bun_zipper_res2.ply",
        scene,
        materials,
        textures);
    scene[0]->matId = 0;
    for (std::size_t i = 0; i < scene.size(); ++i) {
        scene[i]->GLSetup();
    }
}

void App::mainLoop()
{
    //create shader programs using wrapper ShaderProgram class
    std::unordered_map<GLenum, std::string> shaders;
    std::string shadersPath = config["shadersPath"];
    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexPhong_ins.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentPhong.glsl";
    ShaderProgram lightningProgram(shaders);
    GL_CHECK_ERRORS;

    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexLightSource_ins.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentLightSource.glsl";
    ShaderProgram sourceProgram(shaders);
    GL_CHECK_ERRORS;

    //force 60 frames per second
    glfwSwapInterval(1);

    //enabling depth testing
    glEnable(GL_DEPTH_TEST);

    //capture cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //register callbacks
    glfwSetKeyCallback(window, OnKeyboardPressed);
    glfwSetCursorPosCallback(window, OnMouseMove);
    glfwSetMouseButtonCallback(window, OnMouseButtonClicked);
    glfwSetScrollCallback(window, OnMouseScroll);

    std::vector<glm::vec3> cubePositions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f, 3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f, 2.0f, -2.5f),
        glm::vec3(1.5f, 0.2f, -1.5f),
        glm::vec3(-1.3f, 1.0f, -1.5f)
    };
    std::vector<glm::vec3> pointLightPositions = {
        glm::vec3(0.7f, 0.2f, 2.0f),
        glm::vec3(2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f, 2.0f, -12.0f),
        glm::vec3(0.0f, 0.0f, -3.0f)
    };

    //цикл обработки сообщений и отрисовки сцены каждый кадр
    float ratio = static_cast<float>(config["width"]) / static_cast<float>(config["height"]);
    while (!glfwWindowShouldClose(window)) {
        //per-frame time logic
        float currentFrame = glfwGetTime();
        state.deltaTime = currentFrame - state.lastFrame;
        state.lastFrame = currentFrame;

        //handle events
        glfwPollEvents();
        doCameraMovement();

        //очистка и заполнение экрана цветом
        //
        glViewport(0, 0, config["width"], config["height"]);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(lightningProgram.ProgramObj); //StartUseShader

        //model
        glm::mat4 model(1.0);
        //view
        glm::mat4 view = state.camera.GetViewMatrix();
        //projection
        glm::mat4 projection;
        projection = glm::perspective(glm::radians(state.camera.Zoom), ratio, 0.1f, 100.0f);

        //set light sources
        for (std::size_t i = 0; i < pointLightPositions.size(); ++i) {
            std::string idx = std::to_string(i);
            //set point light source
            lightningProgram.SetUniform("pointLights[" + idx + "].position", glm::vec3(view * glm::vec4(pointLightPositions[i], 1.0f)));
            lightningProgram.SetUniform("pointLights[" + idx + "].color", glm::vec3(1.0f));
            lightningProgram.SetUniform("pointLights[" + idx + "].constant", 1.0f);
            lightningProgram.SetUniform("pointLights[" + idx + "].linear", 0.09f);
            lightningProgram.SetUniform("pointLights[" + idx + "].quadratic", 0.032f);
        }

        //set spotlight source
        lightningProgram.SetUniform("spotlightOn", state.isFlashlightOn);
        lightningProgram.SetUniform("spotLight.pointLight.position", glm::vec3(0.0f));
        lightningProgram.SetUniform("spotLight.pointLight.color", glm::vec3(1.0f));
        lightningProgram.SetUniform("spotLight.pointLight.constant", 1.0f);
        lightningProgram.SetUniform("spotLight.pointLight.linear", 0.22f);
        lightningProgram.SetUniform("spotLight.pointLight.quadratic", 0.2f);
        lightningProgram.SetUniform("spotLight.direction", glm::vec3(0.0f, 0.0f, -1.0f));
        lightningProgram.SetUniform("spotLight.cutOff", glm::cos(glm::radians(10.5f)));
        lightningProgram.SetUniform("spotLight.outerCutOff", glm::cos(glm::radians(14.5f)));

        //set directional light source
        lightningProgram.SetUniform("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        lightningProgram.SetUniform("dirLight.color", glm::vec3(0.5f));

        //set material
        uint32_t matId = scene[0]->matId;
        materials[matId].Setup(lightningProgram, textures, GL_TEXTURE0, GL_TEXTURE1, 0, 1);

        std::vector<glm::mat4> modelMatrices(cubePositions.size());
        for (std::size_t i = 0; i < cubePositions.size(); ++i) {
            //model
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            model = glm::scale(model, glm::vec3(10.0f));
            modelMatrices[i] = model;
        }
        //set uniforms with transforms
        lightningProgram.SetUniform("view", view);
        lightningProgram.SetUniform("projection", projection);
        scene[0]->Draw(modelMatrices);

        glUseProgram(sourceProgram.ProgramObj); //StartUseShader

        //color
        sourceProgram.SetUniform("lightColor", glm::vec3(1.0));

        //draw light sources
        modelMatrices.resize(pointLightPositions.size());
        for (std::size_t i = 0; i < pointLightPositions.size(); ++i) {
            //model
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(2.0f));
            modelMatrices[i] = model;
        }
        //set uniforms with transforms
        sourceProgram.SetUniform("view", view);
        sourceProgram.SetUniform("projection", projection);

        scene[0]->Draw(modelMatrices);

        glUseProgram(0); //StopUseShader

        glfwSwapBuffers(window);
    }
    lightningProgram.Release();
    sourceProgram.Release();
}

void App::release()
{
    for (auto& mesh : scene) {
        mesh->Release();
    }
    for (auto& item : textures) {
        item.second->Release();
    }
    glfwTerminate();
}

int App::Run()
{
    int result = createWindow();
    if (result != 0) {
        return result;
    }
    loadModels();
    mainLoop();
    release();
    return 0;
}
