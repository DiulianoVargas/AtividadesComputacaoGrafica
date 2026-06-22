#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

// Coeficientes de reflexão lidos do arquivo .mtl
struct Material
{
    vec3  Ka = vec3(0.2f);        // reflexão ambiente
    vec3  Kd = vec3(0.8f);        // reflexão difusa
    vec3  Ks = vec3(0.5f);        // reflexão especular
    float Ns = 32.0f;             // expoente especular (shininess)
};

int      setupShader();
GLuint   loadSimpleOBJ(const string& filePath, int& nVertices);
Material loadMTL(const string& filePath);
void     key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

const GLuint WIDTH = 800, HEIGHT = 800;

bool rotateX = false, rotateY = true, rotateZ = false;

const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;   // <-- atributo NORMAL vindo do .OBJ (vn)
layout (location = 2) in vec2 texc;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;   // posição do fragmento no espaço de mundo
out vec3 vNormal;   // normal no espaço de mundo

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    fragPos = worldPos.xyz;

    // Corrige a normal para escalas não uniformes (matriz normal)
    vNormal = mat3(transpose(inverse(model))) * normal;

    gl_Position = projection * view * worldPos;
}
)";

const GLchar* fragmentShaderSource = R"(
#version 400
in vec3 fragPos;
in vec3 vNormal;

out vec4 color;

// Coeficientes de reflexão do material (lidos do .mtl)
uniform vec3  Ka;
uniform vec3  Kd;
uniform vec3  Ks;
uniform float Ns;   // expoente especular

uniform vec3 lightPos;     // posição da fonte de luz
uniform vec3 lightColor;   // cor/intensidade da luz
uniform vec3 camPos;       // posição da câmera (para o termo especular)

void main()
{
    // Vetores básicos do modelo de Phong
    vec3 N = normalize(vNormal);                 // normal da superfície
    vec3 L = normalize(lightPos - fragPos);      // direção para a luz
    vec3 V = normalize(camPos  - fragPos);       // direção para o observador
    vec3 R = reflect(-L, N);                      // reflexão da luz sobre a normal

    // Parcela AMBIENTE
    vec3 ambient = Ka * lightColor;

    // Parcela DIFUSA (Lei de Lambert)
    float diff   = max(dot(N, L), 0.0);
    vec3  diffuse = Kd * diff * lightColor;

    // Parcela ESPECULAR (Phong tradicional)
    float spec     = pow(max(dot(R, V), 0.0), Ns);
    vec3  specular = Ks * spec * lightColor;

    vec3 result = ambient + diffuse + specular;
    color = vec4(result, 1.0);
}
)";

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Phong sobre malha .OBJ", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL:   " << glGetString(GL_VERSION)  << endl;

    GLuint shaderID = setupShader();

    int nVertices;
    GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices);
    if (VAO == 0)
    {
        cout << "ERRO: nao foi possivel carregar o .OBJ.\n"
             << "Execute o programa de dentro da pasta build/." << endl;
        return -1;
    }

    Material material = loadMTL("../assets/Modelos3D/Suzanne.mtl");
    cout << "[MTL] Ka(" << material.Ka.r << ") Kd(" << material.Kd.r
         << ") Ks(" << material.Ks.r << ") Ns(" << material.Ns << ")" << endl;

    // Cena: uma luz branca e a câmera
    vec3 lightPos   = vec3(2.0f, 2.0f, 2.0f);
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    vec3 camPos     = vec3(0.0f, 0.0f, 3.0f);

    glUseProgram(shaderID);

    GLint modelLoc      = glGetUniformLocation(shaderID, "model");
    GLint viewLoc       = glGetUniformLocation(shaderID, "view");
    GLint projectionLoc = glGetUniformLocation(shaderID, "projection");

    // Matriz de projeção (perspectiva) e de visualização (câmera fixa)
    mat4 projection = perspective(radians(45.0f),
        (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    mat4 view = lookAt(camPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));
    glUniformMatrix4fv(viewLoc,       1, GL_FALSE, value_ptr(view));

    glUniform3fv(glGetUniformLocation(shaderID, "Ka"), 1, value_ptr(material.Ka));
    glUniform3fv(glGetUniformLocation(shaderID, "Kd"), 1, value_ptr(material.Kd));
    glUniform3fv(glGetUniformLocation(shaderID, "Ks"), 1, value_ptr(material.Ks));
    glUniform1f (glGetUniformLocation(shaderID, "Ns"), material.Ns);

    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"),   1, value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1, value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(shaderID, "camPos"),     1, value_ptr(camPos));

    glEnable(GL_DEPTH_TEST);

    cout << "\n========== CONTROLES ==========\n"
         << " Rotacao: X / Y / Z (liga-desliga giro no eixo)\n"
         << " Sair:    ESC\n"
         << "===============================\n" << endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (float)glfwGetTime();

        mat4 model = mat4(1.0f);
        if (rotateX) model = rotate(model, angle, vec3(1.0f, 0.0f, 0.0f));
        if (rotateY) model = rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
        if (rotateZ) model = rotate(model, angle, vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

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
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_X) rotateX = !rotateX;
    if (key == GLFW_KEY_Y) rotateY = !rotateY;
    if (key == GLFW_KEY_Z) rotateZ = !rotateZ;
}

GLuint loadSimpleOBJ(const string& filePath, int& nVertices)
{
    vector<vec3>    vertices;
    vector<vec2>    texCoords;
    vector<vec3>    normals;
    vector<GLfloat> vBuffer;

    ifstream arq(filePath.c_str());
    if (!arq.is_open())
    {
        cerr << "Erro ao tentar ler o arquivo " << filePath << endl;
        return 0;
    }

    string line;
    while (getline(arq, line))
    {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "v")
        {
            vec3 v;
            ss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt")
        {
            vec2 vt;
            ss >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            vec3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (word == "f")
        {
            // Cada token tem o formato  vi/ti/ni  (índices base-1).
            // Triangula a face em leque caso tenha mais de 3 vértices.
            vector<string> tokens;
            while (ss >> word) tokens.push_back(word);

            for (size_t k = 1; k + 1 < tokens.size(); ++k)
            {
                string tri[3] = { tokens[0], tokens[k], tokens[k + 1] };
                for (const string& tok : tri)
                {
                    int vi = 0, ti = 0, ni = 0;
                    istringstream ts(tok);
                    string idx;

                    if (getline(ts, idx, '/')) vi = idx.empty() ? 0 : stoi(idx) - 1;
                    if (getline(ts, idx, '/')) ti = idx.empty() ? 0 : stoi(idx) - 1;
                    if (getline(ts, idx))      ni = idx.empty() ? 0 : stoi(idx) - 1;

                    // Posição
                    vBuffer.push_back(vertices[vi].x);
                    vBuffer.push_back(vertices[vi].y);
                    vBuffer.push_back(vertices[vi].z);
                    // Normal (vn)
                    vec3 n = normals.empty() ? vec3(0.0f, 0.0f, 1.0f) : normals[ni];
                    vBuffer.push_back(n.x);
                    vBuffer.push_back(n.y);
                    vBuffer.push_back(n.z);
                    // Coordenada de textura
                    vec2 t = texCoords.empty() ? vec2(0.0f) : texCoords[ti];
                    vBuffer.push_back(t.s);
                    vBuffer.push_back(t.t);
                }
            }
        }
    }
    arq.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat),
                 vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    const GLsizei stride = 8 * sizeof(GLfloat);

    // location 0: posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // location 1: normal (nx, ny, nz)  <-- novo atributo
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // location 2: coordenada de textura (s, t)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)(vBuffer.size() / 8); // 8 floats por vértice
    cout << "[OBJ] " << filePath << " -> " << nVertices << " vertices" << endl;
    return VAO;
}

Material loadMTL(const string& filePath)
{
    Material mat;

    ifstream arq(filePath.c_str());
    if (!arq.is_open())
    {
        cerr << "Erro ao tentar ler o material " << filePath
             << " (usando coeficientes padrao)" << endl;
        return mat;
    }

    string line;
    while (getline(arq, line))
    {
        istringstream ss(line);
        string word;
        ss >> word;

        if      (word == "Ka") ss >> mat.Ka.r >> mat.Ka.g >> mat.Ka.b;
        else if (word == "Kd") ss >> mat.Kd.r >> mat.Kd.g >> mat.Kd.b;
        else if (word == "Ks") ss >> mat.Ks.r >> mat.Ks.g >> mat.Ks.b;
        else if (word == "Ns") ss >> mat.Ns;
    }
    arq.close();
    return mat;
}

int setupShader()
{
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
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
