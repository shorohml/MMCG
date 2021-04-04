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
    state.camera.MovementSpeed = 50.0f * 2.0f;
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
    case GLFW_KEY_N: //turn flashlight on/off
        if (action == GLFW_PRESS) {
            state->visualizeNormalsWithColor = !(state->visualizeNormalsWithColor);
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
    //Load sponza scene
    importSceneFromFile(
        std::string(config["dataPath"]) + "/sponza/sponza.obj",
        scene,
        materials,
        textures);
    scene = unifyStaticMeshes(scene, materials);
    scene.push_back(createCube());
}

void App::mainLoop()
{
    //create shader programs using wrapper ShaderProgram class
    std::unordered_map<GLenum, std::string> shaders;
    std::string shadersPath = config["shadersPath"];
    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexPhong.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentPhong.glsl";
    ShaderProgram lightningProgram(shaders);
    GL_CHECK_ERRORS;

    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexLightSource.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentLightSource.glsl";
    ShaderProgram sourceProgram(shaders);
    GL_CHECK_ERRORS;

    //force 60 frames per second
    glfwSwapInterval(1);

    //enabling depth and stencil testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    //enamble anti-aliasing
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

    //capture cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //register callbacks
    glfwSetKeyCallback(window, OnKeyboardPressed);
    glfwSetCursorPosCallback(window, OnMouseMove);
    glfwSetMouseButtonCallback(window, OnMouseButtonClicked);
    glfwSetScrollCallback(window, OnMouseScroll);

    std::vector<glm::vec3> pointLightPositions(5);
    for (std::size_t i = 0; i < pointLightPositions.size(); ++i) {
        pointLightPositions[i] = glm::vec3((float)(i - 2.5f) * 300.0f, 50.0f, 0.0f);
    }
    std::vector<glm::vec3> colors = {
        glm::vec3(1.0f),
        glm::vec3(1.0f),
        glm::vec3(1.0f),
        glm::vec3(1.0f),
        glm::vec3(1.0f)
    };

    //цикл обработки сообщений и отрисовки сцены каждый кадр
    float ratio = static_cast<float>(config["width"]) / static_cast<float>(config["height"]);
    uint32_t frameCount = 0;
    float deltaSum = 0.0f;
    bool firstFrame = true;
    while (!glfwWindowShouldClose(window)) {
        //per-frame time logic
        float currentFrame = glfwGetTime();
        ++frameCount;
        if (firstFrame) {
            //we didn't draw anyting yet, set deltaTime to 0.0f
            state.lastFrame = currentFrame;
            state.deltaTime = 0.0f;
            firstFrame = false;
            frameCount = 0;
        } else {
            //compute deltaTime for last frame
            state.deltaTime = currentFrame - state.lastFrame;
            state.lastFrame = currentFrame;
        }

        //compute and draw FPS
        deltaSum += state.deltaTime;
        if (deltaSum >= printEvery) {
            float fps = static_cast<float>(frameCount) / deltaSum;
            std::string title = std::string(config["name"]) + " FPS: " + std::to_string(fps);
            glfwSetWindowTitle(window, title.c_str());
            deltaSum = 0.0f;
            frameCount = 0;
        }

        //handle events
        glfwPollEvents();
        doCameraMovement();

        //очистка и заполнение экрана цветом
        //
        glViewport(0, 0, config["width"], config["height"]);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        //enamble face culling
        glEnable(GL_CULL_FACE);

        glUseProgram(lightningProgram.ProgramObj); //StartUseShader

        //model
        glm::mat4 model(1.0);
        //view
        glm::mat4 view = state.camera.GetViewMatrix();
        //projection
        glm::mat4 projection;
        projection = glm::perspective(glm::radians(state.camera.Zoom), ratio, 1.0f, 3000.0f);

        lightningProgram.SetUniform("visualizeNormalsWithColor", state.visualizeNormalsWithColor);

        //set light sources
        for (std::size_t i = 0; i < pointLightPositions.size(); ++i) {
            std::string idx = std::to_string(i);
            //set point light source
            glm::vec4 lightPos = view * glm::vec4(pointLightPositions[i], 1.0f);
            lightningProgram.SetUniform("pointLights[" + idx + "].position", glm::vec3(lightPos));
            lightningProgram.SetUniform("pointLights[" + idx + "].ambient", 0.05f * colors[i]);
            lightningProgram.SetUniform("pointLights[" + idx + "].diffuse", 0.8f * colors[i]);
            lightningProgram.SetUniform("pointLights[" + idx + "].specular", glm::vec3(1.0f));
            lightningProgram.SetUniform("pointLights[" + idx + "].constant", 1.0f);
            lightningProgram.SetUniform("pointLights[" + idx + "].linear", 0.0014f);
            lightningProgram.SetUniform("pointLights[" + idx + "].quadratic", 0.000007f);
        }

        //set spotlight source
        lightningProgram.SetUniform("spotlightOn", state.isFlashlightOn);
        lightningProgram.SetUniform("spotLight.pointLight.position", glm::vec3(0.0f));
        lightningProgram.SetUniform("spotLight.pointLight.ambient", glm::vec3(0.05f));
        lightningProgram.SetUniform("spotLight.pointLight.diffuse", glm::vec3(0.9f));
        lightningProgram.SetUniform("spotLight.pointLight.specular", glm::vec3(1.0f));
        lightningProgram.SetUniform("spotLight.pointLight.constant", 1.0f);
        lightningProgram.SetUniform("spotLight.pointLight.linear", 0.0014f);
        lightningProgram.SetUniform("spotLight.pointLight.quadratic", 0.000007f);
        lightningProgram.SetUniform("spotLight.direction", glm::vec3(0.0f, 0.0f, -1.0f));
        lightningProgram.SetUniform("spotLight.cutOff", glm::cos(glm::radians(15.0f)));
        lightningProgram.SetUniform("spotLight.outerCutOff", glm::cos(glm::radians(20.0f)));

        //set directional light source
        glm::vec4 direction = glm::vec4(-0.2f, -1.0f, -0.3f, 0.0f);
        lightningProgram.SetUniform("dirLight.direction", glm::vec3(view * direction));
        lightningProgram.SetUniform("dirLight.ambient", glm::vec3(0.0f));
        lightningProgram.SetUniform("dirLight.diffuse", glm::vec3(0.0f));
        lightningProgram.SetUniform("dirLight.specular", glm::vec3(0.0f));

        lightningProgram.SetUniform("view", view);
        lightningProgram.SetUniform("projection", projection);
        for (std::size_t i = 0; i < scene.size() - 1; ++i) {
            //set material
            uint32_t matId = scene[i]->matId;
            materials[matId].Setup(
                lightningProgram,
                textures,
                GL_TEXTURE0,
                GL_TEXTURE1,
                GL_TEXTURE2,
                0,
                1,
                2);
            //set uniforms with transforms
            lightningProgram.SetUniform("model", scene[i]->model);
            lightningProgram.SetUniform("normalMatrix", glm::mat3(glm::transpose(glm::inverse(view * scene[i]->model))));
            scene[i]->Draw();
        }

        glUseProgram(sourceProgram.ProgramObj); //StartUseShader

        //enamble face culling
        glDisable(GL_CULL_FACE);

        //draw light sources
        for (std::size_t i = 0; i < pointLightPositions.size(); ++i) {
            //model
            model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(10.0f));
            //set uniforms with transforms
            sourceProgram.SetUniform("model", model);
            sourceProgram.SetUniform("view", view);
            sourceProgram.SetUniform("projection", projection);
            //color
            sourceProgram.SetUniform("lightColor", colors[i]);
            scene[scene.size() - 1]->Draw();
        }

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
    loadModels();
    int result = createWindow();
    if (result != 0) {
        return result;
    }
    for (std::size_t i = 0; i < scene.size(); ++i) {
        scene[i]->GLLoad();
    }
    for (auto& item : textures) {
        item.second->GLLoad();
    }
    mainLoop();
    release();
    return 0;
}
