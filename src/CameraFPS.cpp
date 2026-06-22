/* Câmera em Primeira Pessoa
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 *
 * Implementa câmera FPS com classe Camera encapsulando Move e Rotate.
 * Cena com sprites texturizados usando assets/tex/pixelWall.png.
 */

#include <iostream>
#include <string>
#include <vector>
#include <cassert>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

class Camera {
public:
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;

    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    Camera(vec3 startPos = vec3(0.0f, 0.0f, 3.0f))
        : position(startPos),
          front(vec3(0.0f, 0.0f, -1.0f)),
          up(vec3(0.0f, 1.0f, 0.0f)),
          yaw(-90.0f), pitch(0.0f),
          speed(3.0f), sensitivity(0.08f)
    {
        updateVectors();
    }

    mat4 getViewMatrix() const {
        return lookAt(position, position + front, up);
    }

    // direction: 0=frente, 1=trás, 2=esquerda, 3=direita
    void move(int direction, float deltaTime) {
        float velocity = speed * deltaTime;
        switch (direction) {
            case 0: position += front * velocity;  break;
            case 1: position -= front * velocity;  break;
            case 2: position -= right * velocity;  break;
            case 3: position += right * velocity;  break;
        }
    }

    void rotate(float xOffset, float yOffset) {
        yaw   += xOffset * sensitivity;
        pitch += yOffset * sensitivity;

        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        updateVectors();
    }

private:
    void updateVectors() {
        vec3 f;
        f.x = cos(radians(yaw)) * cos(radians(pitch));
        f.y = sin(radians(pitch));
        f.z = sin(radians(yaw)) * cos(radians(pitch));
        front = normalize(f);
        right = normalize(cross(front, vec3(0.0f, 1.0f, 0.0f)));
        up    = normalize(cross(right, front));
    }
};

const GLuint WIDTH = 1000, HEIGHT = 800;

Camera camera(vec3(0.0f, 0.0f, 3.0f));

float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool  firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 vTexCoord;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    vTexCoord = texCoord;
}
)";

const GLchar* fragmentShaderSource = R"(
#version 400
in vec2 vTexCoord;
uniform sampler2D texBuff;
out vec4 color;

void main()
{
    color = texture(texBuff, vTexCoord);
}
)";

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
int  setupShader();
GLuint setupSpriteVAO();
GLuint loadTexture(const string& filePath);
void processInput(GLFWwindow* window);

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Câmera FPS - Computação Gráfica", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version: " << version << endl;

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    GLuint shaderID = setupShader();
    GLuint spriteVAO = setupSpriteVAO();
    GLuint texID = loadTexture("../assets/tex/pixelWall.png");

    glEnable(GL_DEPTH_TEST);

    // Posições dos sprites na cena (paredes ao redor)
    vector<vec3> spritePositions = {
        vec3( 0.0f,  0.0f, -5.0f),
        vec3( 3.0f,  0.0f, -3.0f),
        vec3(-3.0f,  0.0f, -3.0f),
        vec3( 5.0f,  0.0f,  0.0f),
        vec3(-5.0f,  0.0f,  0.0f),
        vec3( 0.0f,  0.0f,  5.0f),
        vec3( 2.0f,  0.0f,  2.0f),
        vec3(-2.0f,  0.0f, -2.0f),
    };

    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));

        glBindVertexArray(spriteVAO);

        for (const vec3& pos : spritePositions) {
            mat4 model = mat4(1.0f);
            model = translate(model, pos);
            model = scale(model, vec3(2.0f, 2.0f, 1.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &spriteVAO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.move(0, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.move(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.move(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.move(3, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xOffset =  (float)xpos - lastX;
    float yOffset =  lastY - (float)ypos; // invertido: y cresce para baixo na tela

    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.rotate(xOffset, yOffset);
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint setupSpriteVAO()
{
    // Quad unitário centrado na origem, no plano XY
    // x, y, z,  s, t
    GLfloat vertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,

        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // posição (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // texCoord (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

GLuint loadTexture(const string& filePath)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        cout << "Falha ao carregar textura: " << filePath << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}
