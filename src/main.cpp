// Initial version taken from https://github.com/v-san/ogl-samples

#include "ShaderProgram.h"

#include <GLFW/glfw3.h>
#include <random>

static const GLsizei WIDTH = 640, HEIGHT = 480; //размеры окна

int initGL()
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

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

int main()
{
    if (!glfwInit())
        return -1;

    //запрашиваем контекст opengl версии 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL basic sample", nullptr, nullptr);
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

    //Reset any OpenGL errors which could be present for some reason
    GLenum gl_error = glGetError();
    while (gl_error != GL_NO_ERROR) {
        gl_error = glGetError();
    }

    //создание шейдерной программы из двух файлов с исходниками шейдеров
    //используется класс-обертка ShaderProgram
    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = "vertex.glsl";
    shaders[GL_FRAGMENT_SHADER] = "fragment.glsl";
    ShaderProgram program(shaders);
    GL_CHECK_ERRORS;

    glfwSwapInterval(1); // force 60 frames per second

    //Создаем и загружаем геометрию поверхности
    //
    GLuint g_vertexBufferObject;
    GLuint g_vertexArrayObject;
    GLuint g_elementBufferObject;
    {
        float vertices[] = {
            0.5f, 0.5f, 0.0f, // top right
            0.5f, -0.5f, 0.0f, // bottom right
            -0.5f, -0.5f, 0.0f, // bottom left
            -0.5f, 0.5f, 0.0f // top left
        };
        unsigned int indices[] = {
            // note that we start from 0!
            0, 1, 3, // first triangle
            1, 2, 3 // second triangle
        };

        g_vertexBufferObject = 0;
        GLuint vertexLocation = 0;

        glGenVertexArrays(1, &g_vertexArrayObject);
        GL_CHECK_ERRORS;
        glBindVertexArray(g_vertexArrayObject);
        GL_CHECK_ERRORS;

        glGenBuffers(1, &g_vertexBufferObject);
        GL_CHECK_ERRORS;
        glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject);
        GL_CHECK_ERRORS;
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        GL_CHECK_ERRORS;

        glGenBuffers(1, &g_elementBufferObject);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_elementBufferObject);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
        GL_CHECK_ERRORS;
        glEnableVertexAttribArray(vertexLocation);
        GL_CHECK_ERRORS;
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    //цикл обработки сообщений и отрисовки сцены каждый кадр
    while (!glfwWindowShouldClose(window)) {
        // input
        processInput(window);

        //очистка и заполнение экрана цветом
        //
        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(program.programObj); // StartUseShader

        glBindVertexArray(g_vertexArrayObject);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glUseProgram(0); //StopUseShader

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    //очищаем vbo, vao и ebo перед закрытием программы
    //
    glDeleteBuffers(1, &g_vertexBufferObject);
    glDeleteBuffers(1, &g_vertexArrayObject);
    glDeleteBuffers(1, &g_elementBufferObject);

    glfwTerminate();
    return 0;
}
