/* SuzanneTex - Renderização da Suzanne com textura
 *
 * Baseado em Hello3D.cpp e LoadSimpleOBJ.cpp
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 *
 * Teclas:
 *   X - rotacionar em torno do eixo X
 *   Y - rotacionar em torno do eixo Y
 *   Z - rotacionar em torno do eixo Z
 *   ESC - fechar
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int loadSimpleOBJ(string filePath, int &nVertices);
GLuint loadTexture(string filePath);

const GLuint WIDTH = 1000, HEIGHT = 1000;

const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 texCoord;
void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    texCoord = texc;
})";

const GLchar* fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
out vec4 color;
void main()
{
    color = texture(texBuff, texCoord);
})";

bool rotateX = false, rotateY = false, rotateZ = false;

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho Renderização de Textura", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    int nVertices;
    GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices);

    GLuint texID = loadTexture("../assets/Modelos3D/Suzanne.png");

    glUseProgram(shaderID);

    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    // Projeção perspectiva
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Câmera fixa levemente afastada
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (GLfloat)glfwGetTime();
        glm::mat4 model = glm::mat4(1.0f);

        if (rotateX)
            model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
        else if (rotateY)
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        else if (rotateZ)
            model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        rotateX = true; rotateY = false; rotateZ = false;
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        rotateX = false; rotateY = true; rotateZ = false;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        rotateX = false; rotateY = false; rotateZ = true;
    }
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int loadSimpleOBJ(string filePath, int &nVertices)
{
    vector<glm::vec3> positions;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;

    ifstream arqEntrada(filePath.c_str());
    if (!arqEntrada.is_open())
    {
        cerr << "Erro ao tentar ler o arquivo " << filePath << endl;
        return -1;
    }

    string line;
    while (getline(arqEntrada, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "v")
        {
            glm::vec3 v;
            ssline >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 vn;
            ssline >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (word == "f")
        {
            while (ssline >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                istringstream ss(word);
                string index;

                if (getline(ss, index, '/')) vi = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index, '/')) ti = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index))      ni = !index.empty() ? stoi(index) - 1 : 0;

                vBuffer.push_back(positions[vi].x);
                vBuffer.push_back(positions[vi].y);
                vBuffer.push_back(positions[vi].z);

                if (!texCoords.empty())
                {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                }
                else
                {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
            }
        }
    }
    arqEntrada.close();

    cout << "Gerando o buffer de geometria..." << endl;

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 5; // x, y, z, s, t
    return VAO;
}

GLuint loadTexture(string filePath)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;

    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 4);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        cout << "Textura carregada: " << filePath << " (" << width << "x" << height << ")" << endl;
    }
    else
    {
        cout << "Falha ao carregar textura: " << filePath << endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}
