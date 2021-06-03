#include "App.h"
#include "Models/ImportScene.h"
#include "ShaderProgram.h"
#include "Simulation/Cloth.h"
#include <map>

App::App(const std::string& pathToConfig)
{
    std::ifstream input(pathToConfig);
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
    dir = glm::normalize(glm::vec3(1.0f, -4.5f, -1.25f));
    glm::vec3 pos = -2100.0f * dir;
    glm::mat4 lightView = glm::lookAt(
        pos,
        pos + dir,
        glm::vec3(0.0f, 0.0f, -1.0f));
    glm::mat4 orthoProjection = glm::ortho(
        -2500.0f,
        2500.0f,
        -1700.0f,
        1700.0f,
        0.0f,
        3000.0f);
    lightSpaceMatrix = orthoProjection * lightView;
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
    case GLFW_KEY_1: //shadow map
        if (action == GLFW_PRESS) {
            if (state->renderingMode == RenderingMode::SHADOW_MAP) {
                state->renderingMode = RenderingMode::DEFAULT;
            } else {
                state->renderingMode = RenderingMode::SHADOW_MAP;
            }
        }
        break;
    case GLFW_KEY_2: //normals
        if (action == GLFW_PRESS) {
            if (state->renderingMode == RenderingMode::NORMALS_COLOR) {
                state->renderingMode = RenderingMode::DEFAULT;
            } else {
                state->renderingMode = RenderingMode::NORMALS_COLOR;
            }
        }
        break;
    case GLFW_KEY_3: //edge detection
        if (action == GLFW_PRESS) {
            if (state->renderingMode == RenderingMode::EDGE_DETECTION) {
                state->renderingMode = RenderingMode::DEFAULT;
            } else {
                state->renderingMode = RenderingMode::EDGE_DETECTION;
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
    // for (auto& mesh : scene) {
    //     if (materials[mesh->matId].name == std::string("flagpole")) {
    //         mesh->isStatic = false; // need to keep flagpoles separate;
    //     }
    // }
    // scene = unifyStaticMeshes(scene, materials);

    //separate meshes with and without face culling
    for (std::size_t i = 0; i < scene.size() - 1; ++i) {
        uint32_t matId = scene[i]->matId;
        if (materials[matId].twosided) {
            twosided.push_back(i);
        } else {
            notTwosided.push_back(i);
        }
    }

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

void App::setupColorBuffer()
{
    glGenFramebuffers(1, &colorBufferFBO);
    GL_CHECK_ERRORS;
    glBindFramebuffer(GL_FRAMEBUFFER, colorBufferFBO);
    GL_CHECK_ERRORS;

    //create texture for color buffer and attach it to the framebuffer
    glGenTextures(1, &colorBufferTexture);
    GL_CHECK_ERRORS;
    glBindTexture(GL_TEXTURE_2D, colorBufferTexture);
    GL_CHECK_ERRORS;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, config["width"], config["height"], 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_ERRORS;
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_ERRORS;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTexture, 0);
    GL_CHECK_ERRORS;

    //create renderbuffer for depth and stencil buffers and attach it to the framebuffer
    glGenRenderbuffers(1, &colorBufferRBO);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, colorBufferRBO);
    GL_CHECK_ERRORS;
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, config["width"], config["height"]);
    GL_CHECK_ERRORS;
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GL_CHECK_ERRORS;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, colorBufferRBO);
    GL_CHECK_ERRORS;
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Couldn't create framebuffer");
    }

    //bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERRORS;
}

void App::deleteColorBuffer()
{
    glDeleteTextures(1, &colorBufferTexture);
    GL_CHECK_ERRORS;
    glDeleteRenderbuffers(1, &colorBufferRBO);
    GL_CHECK_ERRORS;
    glDeleteFramebuffers(1, &colorBufferFBO);
    GL_CHECK_ERRORS;
}

void App::setupShadowMapBuffer()
{
    glGenFramebuffers(1, &shadowMapFBO);
    GL_CHECK_ERRORS;
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    GL_CHECK_ERRORS;

    //generate texture for depth map and attach it to framebuffer
    glGenTextures(1, &shadowMapTexture);
    GL_CHECK_ERRORS;
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    GL_CHECK_ERRORS;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GL_CHECK_ERRORS;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    GL_CHECK_ERRORS;
    glDrawBuffer(GL_NONE);
    GL_CHECK_ERRORS;
    glReadBuffer(GL_NONE);
    GL_CHECK_ERRORS;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Couldn't create framebuffer");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERRORS;
}

void App::deleteShadowMapBuffer()
{
    glDeleteTextures(1, &shadowMapTexture);
    GL_CHECK_ERRORS;
    glDeleteFramebuffers(1, &shadowMapFBO);
    GL_CHECK_ERRORS;
}

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

void App::renderShadowMap(ShaderProgram& depthProgram)
{
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

    //enabling depth testing
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, shadowMapWidth, shadowMapHeight);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(depthProgram.ProgramObj); //StartUseShader

    depthProgram.SetUniform("lightSpaceMatrix", lightSpaceMatrix);
    for (std::size_t i = 0; i < scene.size(); ++i) {
        depthProgram.SetUniform("model", scene[i]->model);
        scene[i]->Draw();
    }

    glUseProgram(0); //StoptUseShader
}

void App::visualizeShadowMap(ShaderProgram& quadDepthProgram)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, config["width"], config["height"]);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(quadDepthProgram.ProgramObj); //StartUseShader

    //set color bufer texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    quadDepthProgram.SetUniform("shadowMap", 0);

    glBindVertexArray(quadVAO);
    GL_CHECK_ERRORS;
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    GL_CHECK_ERRORS;
    glBindVertexArray(0);
    GL_CHECK_ERRORS;

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0); //StopUseShader
}

void App::renderScene(ShaderProgram& lightningProgram)
{
    glBindFramebuffer(GL_FRAMEBUFFER, colorBufferFBO);

    //enabling depth testing
    glEnable(GL_DEPTH_TEST);

    //clear screen and then fill it with color
    glViewport(0, 0, config["width"], config["height"]);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
    glm::vec4 direction = glm::vec4(dir, 0.0f);
    lightningProgram.SetUniform("dirLight.direction", glm::vec3(view * direction));
    lightningProgram.SetUniform("dirLight.ambient", glm::vec3(0.3f));
    lightningProgram.SetUniform("dirLight.diffuse", glm::vec3(0.8f));
    lightningProgram.SetUniform("dirLight.specular", glm::vec3(1.0f));

    lightningProgram.SetUniform("view", view);
    lightningProgram.SetUniform("projection", projection);
    lightningProgram.SetUniform("lightSpaceMatrix", lightSpaceMatrix);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    lightningProgram.SetUniform("shadowMap", 3);

    //opaque objects
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
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(view * scene[i]->model));
        lightningProgram.SetUniform("model", scene[i]->model);
        lightningProgram.SetUniform("normalMatrix", normalMatrix);
        scene[i]->Draw();
    }

    //transparent objects
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
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(view * scene[i]->model));
        lightningProgram.SetUniform("model", scene[i]->model);
        lightningProgram.SetUniform("normalMatrix", normalMatrix);
        scene[i]->Draw();
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0); //StoptUseShader
}

void App::visualizeScene(ShaderProgram& quadColorProgram)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //disable depth testing
    glDisable(GL_DEPTH_TEST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(quadColorProgram.ProgramObj); //StartUseShader

    //set color bufer texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBufferTexture);
    quadColorProgram.SetUniform("colorBuffer", 0);
    quadColorProgram.SetUniform("edgeDetection", state.renderingMode == RenderingMode::EDGE_DETECTION);

    glBindVertexArray(quadVAO);
    GL_CHECK_ERRORS;
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    GL_CHECK_ERRORS;
    glBindVertexArray(0);
    GL_CHECK_ERRORS;

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0); //StopUseShader
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
    setupQuad();

    //enable anti-aliasing
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

    glm::dvec3 poleLeft;
    glm::dvec3 poleRight;
    for (auto& mesh : scene) {
        if (materials[mesh->matId].name == std::string("flagpole")) {
            auto bbox = mesh->GetAABBOX();
            if (bbox.max.z - bbox.min.z < 190.0f) {
                continue;
            }
            //select left- and right-most vertices
            double minZ = mesh->positions[0].z;
            double maxZ = mesh->positions[0].z;
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
            break;
        }
    }
    auto dir = poleRight - poleLeft;
    auto len = glm::length(dir);
    dir = glm::normalize(dir);
    poleLeft += len / 5 * dir;
    poleRight -= len / 5 * dir;

    //create cloth
    Cloth cloth(
        poleLeft,
        poleRight,
        150.0f,
        30,
        45);
    std::vector<glm::dvec3> accelerations = {
        glm::dvec3(0.0, -9.8, 0.0),
        glm::dvec3(7.0, 0.0, 5.0) //wind force
    };
    for (auto& mat : materials) {
        if (mat.second.name == std::string("fabric_a")) {
            cloth.mesh1->matId = mat.first;
            cloth.mesh2->matId = mat.first;
        }
    }
    cloth.mesh1->GLLoad();
    cloth.mesh2->GLLoad();
    scene.push_back(cloth.mesh1);
    scene.push_back(cloth.mesh2);
    twosided.push_back(scene.size() - 2);
    twosided.push_back(scene.size() - 1);

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
            std::string title = std::string(config["name"]) + " FPS: " + std::to_string(fps);
            glfwSetWindowTitle(window, title.c_str());
            deltaSum = 0.0f;
            frameCount = 0;
        }

        //handle events
        glfwPollEvents();
        doCameraMovement();

        //recompute wind force
        accelerations[1].x = 7.0 * sin(currentFrame / 3.0);
        accelerations[1].z = 5.0 * sin(currentFrame);
        //simulate cloth movement
        for (std::uint32_t i = 0; i < 30; ++i) {
            cloth.simulate(
                state.deltaTime * 5,
                30,
                accelerations);
        }
        cloth.recomputePositionsNormals();
        cloth.mesh1->GLUpdatePositionsNormals();
        cloth.mesh2->GLUpdatePositionsNormals();

        //render shadow map to shadowMapTexture
        renderShadowMap(depthProgram);

        // visualize shadow map
        if (state.renderingMode == RenderingMode::SHADOW_MAP) {
            //draw texture with shadow map to quad
            visualizeShadowMap(quadDepthProgram);
            glfwSwapBuffers(window);
            continue;
        }

        //render scene to to colorBufferTexture
        renderScene(lightningProgram);

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
