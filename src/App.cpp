#include "App.h"
#include "Models/ImportScene.h"
#include "ShaderProgram.h"
#include "Simulation/Cloth.h"
#include <map>
#include <sstream>

App::App(const std::string& pathToConfig)
    : sideSplit(2)
{
    std::ifstream input(pathToConfig);
    if (!input.good()) {
        throw std::runtime_error("Failed to load config");
    }
    input >> config;

    //setup initial state
    state.lastX = static_cast<float>(config["width"]) / 2.0f;
    state.lastY = static_cast<float>(config["height"]) / 2.0f;
    state.camera.MouseSensitivity = config["MouseSensitivity"];
    state.camera.MovementSpeed = 50.0f * 2.0f;

    //setup path to directory with shaders
    shadersPath = config["shadersPath"];

    //setup light source for shadow mapping
    //TODO: move this to some separate method for scene setup
    lightDir = glm::normalize(glm::vec3(1.0f, -4.5f, -1.25f));
    glm::vec3 pos = -2100.0f * lightDir;
    glm::mat4 lightView = glm::lookAt(
        pos,
        pos + lightDir,
        glm::vec3(0.0f, 0.0f, -1.0f));
    glm::mat4 orthoProjection = glm::ortho(
        -1900.0f,
        2100.0f,
        -1100.0f,
        1500.0f,
        0.0f,
        2500.0f);
    lightSpaceMatrix = orthoProjection * lightView;

    //setup point light source for shadow mapping
    //TODO: move this to some separate method for scene setup
    float aspect = static_cast<float>(pointShadowMapWidth) / static_cast<float>(pointShadowMapHeight);
    nearPlane = 1.0f;
    farPlane = 2000.0f;
    lightPos = std::vector<glm::vec3>(
        { glm::vec3(-619.532f, 155.27f, 144.924f),
            glm::vec3(485.423f, 163.438f, 142.195f),
            glm::vec3(-1325.59f, 750.0f, -531.42f),
            glm::vec3(-1327.28f, 750.0f, 483.289f),
            glm::vec3(1261.95f, 750.0f, 530.08f),
            glm::vec3(1253.91f, 750.0f, -601.096f) });
    lightColors = std::vector<glm::vec3>(lightPos.size(), glm::vec3(1.0f));
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, nearPlane, farPlane);
    for (std::uint32_t i = 0; i < lightPos.size(); ++i) {
        std::vector<glm::mat4> transforms;
        transforms.push_back(shadowProj * glm::lookAt(lightPos[i], lightPos[i] + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        transforms.push_back(shadowProj * glm::lookAt(lightPos[i], lightPos[i] + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        transforms.push_back(shadowProj * glm::lookAt(lightPos[i], lightPos[i] + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        transforms.push_back(shadowProj * glm::lookAt(lightPos[i], lightPos[i] + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        transforms.push_back(shadowProj * glm::lookAt(lightPos[i], lightPos[i] + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        transforms.push_back(shadowProj * glm::lookAt(lightPos[i], lightPos[i] + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
        lightSpaceTransforms.push_back(transforms);
    }
}

int App::initGL() const
{
    //load opengl functions using GLAD
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

    //request opengl context
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
    case GLFW_KEY_ESCAPE: //exit program
        if (action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        break;
    case GLFW_KEY_SPACE: //switch polygon mode
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
    case GLFW_KEY_C: //print out camera position
        if (action != GLFW_PRESS) {
            break;
        }
        std::cout << "Camera postion: ";
        std::cout << state->camera.Position.x << "f, " << state->camera.Position.y << "f, " << state->camera.Position.z << 'f' << std::endl;
        std::cout << "yaw = " << state->camera.Yaw << ", pitch = " << state->camera.Pitch << std::endl;
        break;
    case GLFW_KEY_1: //default rendring
        if (action == GLFW_PRESS) {
            state->renderingMode = RenderingMode::DEFAULT;
        }
        break;
    case GLFW_KEY_2: //shadow map
        if (action == GLFW_PRESS) {
            if (state->renderingMode == RenderingMode::SHADOW_MAP) {
                state->renderingMode = RenderingMode::DEFAULT;
            } else {
                state->renderingMode = RenderingMode::SHADOW_MAP;
            }
        }
        break;
    case GLFW_KEY_3: //normals
        if (action == GLFW_PRESS) {
            if (state->renderingMode == RenderingMode::NORMALS_COLOR) {
                state->renderingMode = RenderingMode::DEFAULT;
            } else {
                state->renderingMode = RenderingMode::NORMALS_COLOR;
            }
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
    if (scene.size() == 0) {
        throw std::runtime_error("Empty scene, check that git lfs is installed correctly and do 'git lfs pull'");
    }
    //don't unify flagpoles so we can use them separately later
    for (std::uint32_t i = 0; i < scene.size(); ++i) {
        if (materials[scene[i]->matId].name == std::string("flagpole")) {
            scene[i]->isStatic = false;
        }
    }
    scene = unifyStaticMeshes(
        scene,
        materials);
    for (std::uint32_t i = 0; i < scene.size(); ++i) {
        if (materials[scene[i]->matId].name == std::string("flagpole")) {
            scene[i]->isStatic = true;
        }
    }

    //separate meshes with and without face culling
    for (std::size_t i = 0; i < scene.size(); ++i) {
        uint32_t matId = scene[i]->matId;
        if (materials[matId].twosided) {
            sideSplit[0].push_back(i);
        } else {
            sideSplit[1].push_back(i);
        }
    }

    //add mesh for light source
    scene.push_back(createCube());
    lightIdx = scene.size() - 1;
    scene[lightIdx]->name = "lightCube";

    //compute scene bounding box
    AABBOX sceneBBOX = scene[0]->GetAABBOX();
    for (std::size_t i = 1; i < scene.size(); ++i) {
        AABBOX meshBBOX = scene[i]->GetAABBOX();
        sceneBBOX.min = glm::min(sceneBBOX.min, meshBBOX.min);
        sceneBBOX.max = glm::max(sceneBBOX.max, meshBBOX.max);
    }
    std::cout << std::endl;
    std::cout << "Scene bounding box:" << std::endl;
    std::cout << "Min: " << sceneBBOX.min.x << ' ' << sceneBBOX.min.y << ' ' << sceneBBOX.min.z << std::endl;
    std::cout << "Max: " << sceneBBOX.max.x << ' ' << sceneBBOX.max.y << ' ' << sceneBBOX.max.z << std::endl;
    std::cout << std::endl;
}

void App::setupShadowMapBuffer()
{
    glGenFramebuffers(1, &shadowMapFBO);
    GL_CHECK_ERRORS;
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    GL_CHECK_ERRORS;

    //generate textures for depth map (we need three for filtering - original, x-axis pass, y-axis pass)
    //and attach it to framebuffer
    shadowMapTextures = std::vector<GLuint>(2);
    for (std::uint32_t i = 0; i < 2; ++i) {
        glGenTextures(1, &shadowMapTextures[i]);
        GL_CHECK_ERRORS;
        glBindTexture(GL_TEXTURE_2D, shadowMapTextures[i]);
        GL_CHECK_ERRORS;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, shadowMapWidth, shadowMapHeight, 0, GL_RG, GL_FLOAT, nullptr);
        GL_CHECK_ERRORS;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        GL_CHECK_ERRORS;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GL_CHECK_ERRORS;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_ERRORS;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_ERRORS;
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTextures[0], 0);
    GL_CHECK_ERRORS;

    glGenRenderbuffers(1, &shadowMapRBO);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, shadowMapRBO);
    GL_CHECK_ERRORS;
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, shadowMapWidth, shadowMapHeight);
    GL_CHECK_ERRORS;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, shadowMapRBO);
    GL_CHECK_ERRORS;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Couldn't create framebuffer");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GL_CHECK_ERRORS;
}

void App::deleteShadowMapBuffer()
{
    glDeleteTextures(2, shadowMapTextures.data());
    GL_CHECK_ERRORS;
    glDeleteRenderbuffers(1, &shadowMapRBO);
    GL_CHECK_ERRORS;
    glDeleteFramebuffers(1, &shadowMapFBO);
    GL_CHECK_ERRORS;
}

void App::renderShadowMap(ShaderProgram& depthProgram, ShaderProgram& quadDepthProgram)
{
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTextures[0], 0);
    GL_CHECK_ERRORS;

    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, shadowMapWidth, shadowMapHeight);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(depthProgram.ProgramObj); //StartUseShader

    depthProgram.SetUniform("lightSpaceMatrix", lightSpaceMatrix);
    for (std::size_t i = 0; i < scene.size(); ++i) {
        if (duplicatedModels.count(i)) {
            //TODO: use instancing here
            for (auto& model : duplicatedModels[i]) {
                depthProgram.SetUniform("model", model);
                scene[i]->Draw();
            }
        } else {
            depthProgram.SetUniform("model", scene[i]->model);
            scene[i]->Draw();
        }
    }

    //smooth using gaussian filter
    glUseProgram(quadDepthProgram.ProgramObj); //StartUseShader

    //x-direction
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTextures[1], 0);
    GL_CHECK_ERRORS;

    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMapTextures[0]);
    quadDepthProgram.SetUniform("shadowMap", 0);
    quadDepthProgram.SetUniform("gaussFilter", true);
    quadDepthProgram.SetUniform("direction", true);

    glBindVertexArray(quadVAO);
    GL_CHECK_ERRORS;
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    GL_CHECK_ERRORS;
    glBindVertexArray(0);
    GL_CHECK_ERRORS;

    //y-direction
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTextures[0], 0);
    GL_CHECK_ERRORS;

    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMapTextures[1]);
    quadDepthProgram.SetUniform("shadowMap", 0);
    quadDepthProgram.SetUniform("gaussFilter", true);
    quadDepthProgram.SetUniform("direction", false);

    glBindVertexArray(quadVAO);
    GL_CHECK_ERRORS;
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    GL_CHECK_ERRORS;
    glBindVertexArray(0);
    GL_CHECK_ERRORS;

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0); //StoptUseShader
}

void App::visualizeShadowMap(ShaderProgram& quadDepthProgram)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, config["width"], config["height"]);

    glUseProgram(quadDepthProgram.ProgramObj); //StartUseShader

    //set color bufer texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMapTextures[0]);
    quadDepthProgram.SetUniform("shadowMap", 0);

    quadDepthProgram.SetUniform("gaussFilter", false);
    quadDepthProgram.SetUniform("direction", true);

    glBindVertexArray(quadVAO);
    GL_CHECK_ERRORS;
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    GL_CHECK_ERRORS;
    glBindVertexArray(0);
    GL_CHECK_ERRORS;

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0); //StopUseShader
}

void App::setupPointShadowMapBuffer()
{
    glGenFramebuffers(1, &pointShadowMapFBO);
    GL_CHECK_ERRORS;
    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowMapFBO);
    GL_CHECK_ERRORS;

    //create cubemap texture
    pointShadowMapTextures.resize(lightPos.size());
    for (std::uint32_t i = 0; i < lightPos.size(); ++i) {
        glGenTextures(1, &pointShadowMapTextures[i]);
        GL_CHECK_ERRORS;
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowMapTextures[i]);
        for (std::uint32_t j = 0; j < 6; ++j) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT,
                pointShadowMapWidth, pointShadowMapHeight, 0,
                GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        GL_CHECK_ERRORS;
    }

    //attach texture to framebuffer
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointShadowMapTextures[0], 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Couldn't create framebuffer");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERRORS;
}

void App::deletePointShadowMapBuffer()
{
    glDeleteTextures(lightPos.size(), pointShadowMapTextures.data());
    glDeleteFramebuffers(1, &pointShadowMapFBO);
    GL_CHECK_ERRORS;
}

void App::renderPointShadowMap(ShaderProgram& depthProgram)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowMapFBO);

    for (std::uint32_t i = 0; i < lightPos.size(); ++i) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointShadowMapTextures[i], 0);

        glEnable(GL_DEPTH_TEST);

        glViewport(0, 0, pointShadowMapWidth, pointShadowMapHeight);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(depthProgram.ProgramObj); //StartUseShader

        for (std::uint32_t j = 0; j < lightSpaceTransforms[i].size(); ++j) {
            depthProgram.SetUniform(
                "lightSpaceTransforms[" + std::to_string(j) + "]",
                lightSpaceTransforms[i][j]);
        }
        depthProgram.SetUniform("lightPos", lightPos[i]);
        depthProgram.SetUniform("farPlane", farPlane);

        for (std::size_t j = 0; j < scene.size(); ++j) {
            if (duplicatedModels.count(j)) {
                //TODO: use instancing here
                for (auto& model : duplicatedModels[j]) {
                    depthProgram.SetUniform("model", model);
                    scene[j]->Draw();
                }
            } else {
                depthProgram.SetUniform("model", scene[j]->model);
                scene[j]->Draw();
            }
        }

        glUseProgram(0); //StopUseShader
    }
}

//TODO: Move this to Mesh.cpp
void App::setupQuad()
{
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
    glGenVertexArrays(1, &quadVAO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &quadVBO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &quadEBO);
    GL_CHECK_ERRORS;
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
}

void App::deleteQuad()
{
    glDeleteBuffers(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &quadEBO);
}

void App::setupColorBuffer()
{
    glGenFramebuffers(1, &colorBufferFBO);
    GL_CHECK_ERRORS;
    glBindFramebuffer(GL_FRAMEBUFFER, colorBufferFBO);
    GL_CHECK_ERRORS;

    //create texture for color buffer and attach it to the framebuffer
    //need two textures for bloom effect
    colorBufferTextures = std::vector<GLuint>(2);
    for (std::uint32_t i = 0; i < 2; ++i) {
        glGenTextures(1, &colorBufferTextures[i]);
        GL_CHECK_ERRORS;
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorBufferTextures[i]);
        GL_CHECK_ERRORS;
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB16F, config["width"], config["height"], GL_TRUE);
        GL_CHECK_ERRORS;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE, colorBufferTextures[i], 0);
        GL_CHECK_ERRORS;
    }
    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    //create renderbuffer for depth and stencil buffers and attach it to the framebuffer
    glGenRenderbuffers(1, &colorBufferRBO);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, colorBufferRBO);
    GL_CHECK_ERRORS;
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, config["width"], config["height"]);
    GL_CHECK_ERRORS;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, colorBufferRBO);
    GL_CHECK_ERRORS;
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Couldn't create framebuffer");
    }

    glGenFramebuffers(1, &pongFBO);
    GL_CHECK_ERRORS;
    glBindFramebuffer(GL_FRAMEBUFFER, pongFBO);
    GL_CHECK_ERRORS;

    //generate textures for depth map (we need three for filtering - original, x-axis pass, y-axis pass)
    //and attach it to framebuffer
    pongTextures = std::vector<GLuint>(2);
    for (std::uint32_t i = 0; i < 2; ++i) {
        glGenTextures(1, &pongTextures[i]);
        GL_CHECK_ERRORS;
        glBindTexture(GL_TEXTURE_2D, pongTextures[i]);
        GL_CHECK_ERRORS;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, config["width"], config["height"], 0, GL_RGB, GL_FLOAT, nullptr);
        GL_CHECK_ERRORS;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        GL_CHECK_ERRORS;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GL_CHECK_ERRORS;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_ERRORS;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_ERRORS;
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pongTextures[0], 0);
    GL_CHECK_ERRORS;

    glGenRenderbuffers(1, &pongRBO);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, pongRBO);
    GL_CHECK_ERRORS;
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, config["width"], config["height"]);
    GL_CHECK_ERRORS;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pongRBO);
    GL_CHECK_ERRORS;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Couldn't create framebuffer");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GL_CHECK_ERRORS;
}

void App::deleteColorBuffer()
{
    glDeleteTextures(colorBufferTextures.size(), colorBufferTextures.data());
    GL_CHECK_ERRORS;
    glDeleteRenderbuffers(1, &colorBufferRBO);
    GL_CHECK_ERRORS;
    glDeleteFramebuffers(1, &colorBufferFBO);
    GL_CHECK_ERRORS;
    glDeleteTextures(pongTextures.size(), pongTextures.data());
    GL_CHECK_ERRORS;
    glDeleteFramebuffers(1, &pongRBO);
    GL_CHECK_ERRORS;
    glDeleteFramebuffers(1, &pongFBO);
    GL_CHECK_ERRORS;
}

void App::renderScene(
    ShaderProgram& lightningProgram,
    ShaderProgram& sourceProgram,
    ShaderProgram& quadColorProgram)
{
    glBindFramebuffer(GL_FRAMEBUFFER, colorBufferFBO);

    glEnable(GL_DEPTH_TEST);

    //clear screen and then fill it with color
    glViewport(0, 0, config["width"], config["height"]);
    static const float color0[] = { 0.53f, 0.81f, 0.92f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, color0);
    static const float color1[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glClearBufferfv(GL_COLOR, 1, color1);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(lightningProgram.ProgramObj); //StartUseShader

    //view
    glm::mat4 view = state.camera.GetViewMatrix();
    //projection
    float ratio = static_cast<float>(config["width"]) / static_cast<float>(config["height"]);
    glm::mat4 projection = glm::perspective(glm::radians(state.camera.Zoom), ratio, 1.0f, 3000.0f);

    lightningProgram.SetUniform("visualizeNormalsWithColor", state.renderingMode == RenderingMode::NORMALS_COLOR);

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
    glm::vec4 direction = glm::vec4(lightDir, 0.0f);
    lightningProgram.SetUniform("dirLight.direction", glm::vec3(view * direction));
    lightningProgram.SetUniform("dirLight.ambient", glm::vec3(0.3f));
    lightningProgram.SetUniform("dirLight.diffuse", glm::vec3(0.9f));
    lightningProgram.SetUniform("dirLight.specular", glm::vec3(0.9f));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadowMapTextures[0]);
    lightningProgram.SetUniform("dirLight.shadowMap", 3);

    lightningProgram.SetUniform("view", view);
    lightningProgram.SetUniform("projection", projection);
    lightningProgram.SetUniform("lightSpaceMatrix", lightSpaceMatrix);

    //set light sources
    lightningProgram.SetUniform("farPlane", farPlane);
    for (std::uint32_t i = 0; i < lightPos.size(); ++i) {
        std::string idx = std::to_string(i);
        //set point light source
        glm::vec4 lightPosView = view * glm::vec4(lightPos[i], 1.0f);
        lightningProgram.SetUniform("pointLights[" + idx + "].position", glm::vec3(lightPosView));
        lightningProgram.SetUniform("pointLights[" + idx + "].positionWorldSpace", lightPos[i]);
        lightningProgram.SetUniform("pointLights[" + idx + "].ambient", 0.1f * lightColors[i]);
        lightningProgram.SetUniform("pointLights[" + idx + "].diffuse", 0.8f * lightColors[i]);
        lightningProgram.SetUniform("pointLights[" + idx + "].specular", glm::vec3(0.8f));
        lightningProgram.SetUniform("pointLights[" + idx + "].constant", 1.0f);
        lightningProgram.SetUniform("pointLights[" + idx + "].linear", 0.0007f);
        lightningProgram.SetUniform("pointLights[" + idx + "].quadratic", 0.000004f);
        glActiveTexture(GL_TEXTURE0 + i + 4);
        GL_CHECK_ERRORS;
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowMapTextures[i]);
        GL_CHECK_ERRORS;
        lightningProgram.SetUniform("pointLights[" + idx + "].pointShadowMap", static_cast<int>(4 + i));
        GL_CHECK_ERRORS;
    }

    for (std::size_t i = 0; i < 2; ++i) {
        if (i == 0) {
            //transparent objects
            glDisable(GL_CULL_FACE);
        } else {
            //opaque objects
            glEnable(GL_CULL_FACE);
        }
        for (std::size_t j : sideSplit[i]) {
            if (j == lightIdx) {
                continue;
            }
            //set material
            uint32_t matId = scene[j]->matId;
            materials[matId].Setup(
                lightningProgram,
                textures,
                GL_TEXTURE0,
                GL_TEXTURE1,
                GL_TEXTURE2,
                0,
                1,
                2);
            if (duplicatedModels.count(j)) {
                for (auto& model : duplicatedModels[j]) {
                    //TODO: use instancing here
                    glm::mat3 normalMatrix = glm::transpose(glm::inverse(view * model));
                    lightningProgram.SetUniform("model", model);
                    lightningProgram.SetUniform("normalMatrix", normalMatrix);
                    scene[j]->Draw();
                }
            } else {
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(view * scene[j]->model));
                lightningProgram.SetUniform("model", scene[j]->model);
                lightningProgram.SetUniform("normalMatrix", normalMatrix);
                scene[j]->Draw();
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    glUseProgram(sourceProgram.ProgramObj); //StartUseShader
    for (std::size_t i = 0; i < lightPos.size(); ++i) {
        //model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos[i]);
        model = glm::scale(model, glm::vec3(10.0f));
        //set uniforms with transforms
        sourceProgram.SetUniform("model", model);
        sourceProgram.SetUniform("view", view);
        sourceProgram.SetUniform("projection", projection);
        //color
        sourceProgram.SetUniform("lightColor", lightColors[i] + glm::vec3(0.1));
        scene[lightIdx]->Draw();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0); //StoptUseShader

    //blit from multisampled texture
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    glBindFramebuffer(GL_FRAMEBUFFER, pongFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pongTextures[0], 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, colorBufferFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pongFBO);
    glBlitFramebuffer(0, 0, config["width"], config["height"], 0, 0, config["width"], config["height"], GL_COLOR_BUFFER_BIT, GL_NEAREST);

    //------------------------------------------------------------------
    //Gaussian smoothing for bloom effect
    //------------------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, pongFBO);
    glUseProgram(quadColorProgram.ProgramObj); //StartUseShader
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(quadVAO);
    for (std::uint32_t i = 0; i < 4; ++i) {
        //x-axis
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pongTextures[1], 0);
        glBindTexture(GL_TEXTURE_2D, pongTextures[0]);
        quadColorProgram.SetUniform("colorBuffer", 0);
        quadColorProgram.SetUniform("gaussFilter", true);
        quadColorProgram.SetUniform("direction", true);
        quadColorProgram.SetUniform("addBloom", false);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        //y-axis
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pongTextures[0], 0);
        glBindTexture(GL_TEXTURE_2D, pongTextures[1]);
        quadColorProgram.SetUniform("direction", false);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0); //StoptUseShader
}

void App::visualizeScene(ShaderProgram& quadColorProgram)
{
    //blit from multisampled texture
    glBindFramebuffer(GL_FRAMEBUFFER, colorBufferFBO);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_FRAMEBUFFER, pongFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pongTextures[1], 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, colorBufferFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pongFBO);
    glBlitFramebuffer(0, 0, config["width"], config["height"], 0, 0, config["width"], config["height"], GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //disable depth testing
    glDisable(GL_DEPTH_TEST);

    glUseProgram(quadColorProgram.ProgramObj); //StartUseShader

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pongTextures[1]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pongTextures[0]);
    quadColorProgram.SetUniform("colorBuffer", 0);
    quadColorProgram.SetUniform("bloomBuffer", 1);
    quadColorProgram.SetUniform("gaussFilter", false);
    quadColorProgram.SetUniform("direction", false);
    quadColorProgram.SetUniform("addBloom", true);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0); //StopUseShader
}

namespace {
std::pair<glm::dvec3, glm::dvec3> findCorners(std::shared_ptr<Mesh>& mesh)
{
    //select left- and right-most vertices
    double minZ = mesh->positions[0].z;
    double maxZ = mesh->positions[0].z;
    glm::dvec3 poleLeft = mesh->positions[0];
    glm::dvec3 poleRight = mesh->positions[0];
    for (std::uint32_t i = 0; i < mesh->positions.size(); ++i) {
        if (mesh->positions[i].z < minZ) {
            minZ = mesh->positions[i].z;
            poleLeft = mesh->positions[i];
        }
        if (mesh->positions[i].z > maxZ) {
            maxZ = mesh->positions[i].z;
            poleRight = mesh->positions[i];
        }
    }
    //adjust length
    auto dir = poleRight - poleLeft;
    auto len = glm::length(dir);
    dir = glm::normalize(dir);
    poleLeft += len / 5 * dir;
    poleRight -= len / 5 * dir;
    return std::make_pair(poleLeft, poleRight);
}

std::unique_ptr<Cloth> createCloth(std::shared_ptr<Mesh>& mesh)
{
    auto corners = findCorners(mesh);
    return std::make_unique<Cloth>(
        corners.first,
        corners.second,
        150.0f,
        20,
        30);
}

glm::mat4 createModelMat(glm::dvec3& orig, std::shared_ptr<Mesh>& mesh)
{
    auto corners = findCorners(mesh);
    glm::mat4 res(1.0f);
    glm::vec3 diff = corners.first - orig;
    res[3][0] = diff[0];
    res[3][1] = diff[1];
    res[3][2] = diff[2];
    return res;
}

//from https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values
template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}
}

void App::mainLoop()
{
    //create shader programs
    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexPhong.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentPhong.glsl";
    ShaderProgram lightningProgram(shaders);
    GL_CHECK_ERRORS;

    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexDepth.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentDepth.glsl";
    ShaderProgram depthProgram(shaders);
    GL_CHECK_ERRORS;

    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexQuad.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentQuadColor.glsl";
    ShaderProgram quadColorProgram(shaders);
    GL_CHECK_ERRORS;

    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexQuad.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentQuadDepth.glsl";
    ShaderProgram quadDepthProgram(shaders);
    GL_CHECK_ERRORS;

    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexLightSource.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentLightSource.glsl";
    ShaderProgram sourceProgram(shaders);
    GL_CHECK_ERRORS;

    shaders[GL_VERTEX_SHADER] = shadersPath + "/vertexPointDepth.glsl";
    shaders[GL_FRAGMENT_SHADER] = shadersPath + "/fragmentPointDepth.glsl";
    shaders[GL_GEOMETRY_SHADER] = shadersPath + "/geometryPointDepth.glsl";
    ShaderProgram pointDepthPorgram(shaders);
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

    //setup framebuffers and quad to render resulting textures
    setupColorBuffer();
    setupShadowMapBuffer();
    setupPointShadowMapBuffer();
    setupQuad();

    //find flagpoles
    std::vector<std::vector<uint32_t>> poles(2);
    for (std::uint32_t i = 0; i < scene.size(); ++i) {
        if (materials[scene[i]->matId].name == std::string("flagpole")) {
            auto bbox = scene[i]->GetAABBOX();
            if (bbox.max.z - bbox.min.z < 190.0f) {
                continue;
            }
            if (bbox.min.z <= -100.0f) {
                poles[0].push_back(i);
            } else {
                poles[1].push_back(i);
            }
        }
    }

    //create cloths (one for left and one for right)
    std::vector<std::unique_ptr<Cloth>> cloths;
    cloths.push_back(createCloth(scene[poles[0][0]]));
    cloths.push_back(createCloth(scene[poles[1][0]]));
    std::vector<std::vector<glm::mat4>> modelMats(2);
    //create models matrices for left and right poles
    for (std::uint32_t i = 0; i < 2; ++i) {
        for (std::uint32_t j : poles[i]) {
            modelMats[i].push_back(createModelMat(
                cloths[i]->upperLeftCorner,
                scene[j]));
        }
    }
    for (std::uint32_t i = 0; i < cloths.size(); ++i) {
        //set material
        for (auto& mat : materials) {
            if (mat.second.name == std::string("fabric_flag")) {
                cloths[i]->mesh1->matId = mat.first;
                cloths[i]->mesh2->matId = mat.first;
            }
        }
        //load to GPU and add to scene
        cloths[i]->mesh1->GLLoad();
        cloths[i]->mesh2->GLLoad();
        scene.push_back(cloths[i]->mesh1);
        scene.push_back(cloths[i]->mesh2);
        sideSplit[0].push_back(scene.size() - 2);
        sideSplit[0].push_back(scene.size() - 1);
        duplicatedModels[scene.size() - 2] = modelMats[i];
        duplicatedModels[scene.size() - 1] = modelMats[i];
    }
    std::vector<glm::dvec3> accelerations = {
        glm::dvec3(0.0, -9.8, 0.0),
        glm::dvec3(0.0) //wind force
    };

    //main loop with scene rendering at every frame
    uint32_t frameCount = 0;
    float deltaSum = 0.0f;
    bool firstFrame = true;
    //TODO: move FPS logic to separate class
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
            std::string title = std::string(config["name"]) + " FPS: " + to_string_with_precision(fps, 1);
            glfwSetWindowTitle(window, title.c_str());
            deltaSum = 0.0f;
            frameCount = 0;
        }

        //handle events
        glfwPollEvents();
        doCameraMovement();

        //recompute wind force
        accelerations[1].x = 7.0 * sin(currentFrame / 3.0);
        accelerations[1].z = 5.0 * sin(currentFrame / 6.0 + 2.0);
        // simulate cloth movement
        // TODO: it's possible to parallelize this
        for (auto& cloth : cloths) {
            double frac = state.deltaTime * 30;
            for (std::uint32_t i = 0; i < 90 * frac; ++i) {
                cloth->simulate(
                    1.0 / 30.0,
                    30,
                    accelerations);
            }
            cloth->recomputePositionsNormals();
            cloth->mesh1->GLUpdatePositionsNormals();
            cloth->mesh2->GLUpdatePositionsNormals();
        }

        //render shadow map to shadowMapTexture
        renderShadowMap(depthProgram, quadDepthProgram);

        //visualize shadow map
        if (state.renderingMode == RenderingMode::SHADOW_MAP) {
            //draw texture with shadow map to quad
            visualizeShadowMap(quadDepthProgram);
            glfwSwapBuffers(window);
            continue;
        }

        //render point shadow map to pointShadowMapTexture
        renderPointShadowMap(pointDepthPorgram);

        //render scene to colorBufferTexture
        renderScene(lightningProgram, sourceProgram, quadColorProgram);

        //draw texture with rendered scene to quad
        if (state.filling == 0) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        visualizeScene(quadColorProgram);
        glfwSwapBuffers(window);
        if (state.filling == 0) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
    }
    lightningProgram.Release();
    depthProgram.Release();
    quadColorProgram.Release();
    quadDepthProgram.Release();
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
    deleteQuad();
    deleteColorBuffer();
    deleteShadowMapBuffer();
    deletePointShadowMapBuffer();
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
