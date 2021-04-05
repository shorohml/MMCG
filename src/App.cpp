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
    case GLFW_KEY_E: //turn edgeDetection on/off
        if (action == GLFW_PRESS) {
            state->edgeDetection = !(state->edgeDetection);
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
    scene[scene.size() - 1]->name = "lightCube";
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

    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexQuad.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentQuad.glsl";
    ShaderProgram quadProgram(shaders);
    GL_CHECK_ERRORS;

    //force 60 frames per second
    glfwSwapInterval(1);

    //capture cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //register callbacks
    glfwSetKeyCallback(window, OnKeyboardPressed);
    glfwSetCursorPosCallback(window, OnMouseMove);
    glfwSetMouseButtonCallback(window, OnMouseButtonClicked);
    glfwSetScrollCallback(window, OnMouseScroll);

    //define point light sources positions
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

    //generate framebuffer
    glGenFramebuffers(1, &FBO);
    GL_CHECK_ERRORS;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    GL_CHECK_ERRORS;

    //create texture for color buffer and attach it to the framebuffer
    glGenTextures(1, &texColorBuffer);
    GL_CHECK_ERRORS;
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    GL_CHECK_ERRORS;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, config["width"], config["height"], 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    GL_CHECK_ERRORS;
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_ERRORS;
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_ERRORS;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);
    GL_CHECK_ERRORS;

    //create renderbuffer for depth and stencil buffers and attach it to the framebuffer
    glGenRenderbuffers(1, &RBO);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    GL_CHECK_ERRORS;
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, config["width"], config["height"]);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GL_CHECK_ERRORS;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
    GL_CHECK_ERRORS;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Couldn't create framebuffer");
    }
    //bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERRORS;

    //create quad for texColorBuffer rendering
    glGenVertexArrays(1, &quadVAO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &quadVBO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &quadEBO);
    GL_CHECK_ERRORS;
    float quadData[16] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    uint32_t indices[6] = {
        0, 2, 3,
        2, 0, 1
    };
    //bind VAO and VBO
    glBindVertexArray(quadVAO);
    GL_CHECK_ERRORS;
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GL_FLOAT), quadData, GL_STATIC_DRAW);
    GL_CHECK_ERRORS;
    //positions
    glEnableVertexAttribArray(0);
    GL_CHECK_ERRORS;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (GLvoid*)0);
    GL_CHECK_ERRORS;
    //texCoords
    glEnableVertexAttribArray(1);
    GL_CHECK_ERRORS;
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (GLvoid*)(2 * sizeof(GL_FLOAT)));
    GL_CHECK_ERRORS;
    //indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    GL_CHECK_ERRORS;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    GL_CHECK_ERRORS;
    //unbind VAO and VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_ERRORS;
    glBindVertexArray(0);
    GL_CHECK_ERRORS;

    //enamble anti-aliasing
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

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

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        // draw scene
        //--------------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------------

        //enabling depth testing
        glEnable(GL_DEPTH_TEST);

        //очистка и заполнение экрана цветом
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        //separate meshes with and without culling
        std::vector<std::size_t> twosided;
        std::vector<std::size_t> notTwosided;
        for (std::size_t i = 0; i < scene.size() - 1; ++i) {
            uint32_t matId = scene[i]->matId;
            if (materials[matId].twosided) {
                twosided.push_back(i);
            } else {
                notTwosided.push_back(i);
            }
        }

        //enamble face culling
        glEnable(GL_CULL_FACE);
        for (std::size_t i : notTwosided) {
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

        //disable face culling
        glDisable(GL_CULL_FACE);
        for (std::size_t i : twosided) {
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
        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(sourceProgram.ProgramObj); //StartUseShader

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

        //--------------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------------

        //bind default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //disable depth testing
        glDisable(GL_DEPTH_TEST);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(quadProgram.ProgramObj); //StartUseShader

        //set color bufer texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texColorBuffer);
        quadProgram.SetUniform("colorBuffer", 0);
        quadProgram.SetUniform("edgeDetection", state.edgeDetection);

        glBindVertexArray(quadVAO);
        GL_CHECK_ERRORS;
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        GL_CHECK_ERRORS;
        glBindVertexArray(0);
        GL_CHECK_ERRORS;

        glUseProgram(0); ////StopUseShader

        glfwSwapBuffers(window);
    }
    lightningProgram.Release();
    sourceProgram.Release();
}

void App::release()
{
    for (auto& mesh : scene) {
        mesh->Release();
        GL_CHECK_ERRORS;
    }
    for (auto& item : textures) {
        item.second->Release();
        GL_CHECK_ERRORS;
    }
    //delete quad
    glDeleteBuffers(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &quadEBO);
    //delete framebuffer
    glDeleteTextures(1, &texColorBuffer);
    GL_CHECK_ERRORS;
    glDeleteRenderbuffers(1, &RBO);
    GL_CHECK_ERRORS;
    glDeleteFramebuffers(1, &FBO);
    GL_CHECK_ERRORS;
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
