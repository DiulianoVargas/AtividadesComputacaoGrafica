/* Trajetorias - Trajetórias cíclicas para objetos 3D
 *
 * Computação Gráfica - Unisinos
 *
 * Controles:
 *   W/A/S/D    - mover câmera FPS
 *   Mouse      - olhar ao redor
 *   TAB        - selecionar próximo objeto
 *   1-9        - selecionar objeto por índice
 *   P          - adicionar posição da câmera como ponto de controle do objeto selecionado
 *   O          - salvar trajetórias em assets/trajetorias.txt
 *   ESC        - fechar
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

    Camera(vec3 startPos = vec3(0.0f, 0.0f, 8.0f))
        : position(startPos),
          front(vec3(0.0f, 0.0f, -1.0f)),
          up(vec3(0.0f, 1.0f, 0.0f)),
          yaw(-90.0f), pitch(0.0f),
          speed(5.0f), sensitivity(0.08f)
    {
        updateVectors();
    }

    mat4 getViewMatrix() const {
        return lookAt(position, position + front, up);
    }

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

struct SceneObject {
    string name;
    string objPath;
    string texPath;
    GLuint vao    = 0;
    GLuint texID  = 0;
    int nVertices = 0;
    vec3 scale    = vec3(1.0f);
    float speed   = 1.5f;

    vector<vec3> controlPoints;
    int   currentPoint = 0;
    float t            = 0.0f;
    vec3  position     = vec3(0.0f);
};

const GLuint WIDTH = 1000, HEIGHT = 800;

Camera camera(vec3(0.0f, 2.0f, 8.0f));

float lastX      = WIDTH  / 2.0f;
float lastY      = HEIGHT / 2.0f;
bool  firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

vector<SceneObject> objects;
int selectedObject = 0;

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
}
)";

const GLchar* fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
uniform float highlight;
out vec4 color;

void main()
{
    vec4 tex = texture(texBuff, texCoord);
    // objeto selecionado recebe leve tint amarelo
    color = mix(tex, vec4(1.0, 1.0, 0.0, 1.0), highlight * 0.25);
}
)";

void   key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void   mouse_callback(GLFWwindow* window, double xpos, double ypos);
void   processInput(GLFWwindow* window);
int    setupShader();
int    loadSimpleOBJ(const string& filePath, int& nVertices);
GLuint loadTexture(const string& filePath);
void   updateTrajectory(SceneObject& obj, float dt);
bool   loadScene(const string& filePath);
void   saveScene(const string& filePath);

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Trajetorias - Computação Gráfica", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL:   " << glGetString(GL_VERSION)  << endl;

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint shaderID = setupShader();

    // Carregar cena do arquivo de configuração
    const string sceneFile = "../assets/trajetorias.txt";
    if (!loadScene(sceneFile)) {
        cout << "Arquivo " << sceneFile << " não encontrado ou vazio. "
             << "Cena padrão será usada." << endl;

        // Cena padrão caso o arquivo não exista
        {
            SceneObject obj;
            obj.name    = "Suzanne";
            obj.objPath = "../assets/Modelos3D/Suzanne.obj";
            obj.texPath = "../assets/Modelos3D/Suzanne.png";
            obj.scale   = vec3(1.0f);
            obj.speed   = 1.5f;
            obj.controlPoints = {
                vec3( 0.0f, 0.0f, -5.0f),
                vec3( 4.0f, 0.0f, -2.0f),
                vec3( 4.0f, 0.0f,  3.0f),
                vec3(-4.0f, 0.0f,  3.0f),
                vec3(-4.0f, 0.0f, -5.0f),
            };
            obj.position = obj.controlPoints[0];
            objects.push_back(obj);
        }
        {
            SceneObject obj;
            obj.name    = "Cubo";
            obj.objPath = "../assets/Modelos3D/Cube.obj";
            obj.texPath = "../assets/tex/pixelWall.png";
            obj.scale   = vec3(0.5f);
            obj.speed   = 2.0f;
            obj.controlPoints = {
                vec3( 0.0f,  2.0f, 0.0f),
                vec3( 3.0f,  2.0f, 0.0f),
                vec3( 3.0f, -2.0f, 0.0f),
                vec3( 0.0f, -2.0f, 0.0f),
            };
            obj.position = obj.controlPoints[0];
            objects.push_back(obj);
        }
    }

    // Carregar geometria e texturas para cada objeto
    for (SceneObject& obj : objects) {
        int nVerts = 0;
        GLuint vao = loadSimpleOBJ(obj.objPath, nVerts);
        if (vao == 0) {
            cout << "AVISO: não foi possível carregar " << obj.objPath << endl;
        }
        obj.vao       = vao;
        obj.nVertices = nVerts;
        obj.texID     = loadTexture(obj.texPath);

        if (!obj.controlPoints.empty())
            obj.position = obj.controlPoints[0];
    }

    glEnable(GL_DEPTH_TEST);

    mat4 projection = perspective(radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    GLint modelLoc     = glGetUniformLocation(shaderID, "model");
    GLint viewLoc      = glGetUniformLocation(shaderID, "view");
    GLint highlightLoc = glGetUniformLocation(shaderID, "highlight");

    cout << "\n=== Controles ===" << endl;
    cout << "  W/A/S/D  - mover câmera" << endl;
    cout << "  Mouse    - olhar" << endl;
    cout << "  TAB      - selecionar próximo objeto" << endl;
    cout << "  1-9      - selecionar objeto por índice" << endl;
    cout << "  P        - adicionar ponto de controle (posição da câmera)" << endl;
    cout << "  O        - salvar trajetórias" << endl;
    cout << "  ESC      - fechar" << endl;
    cout << "=================" << endl;

    if (!objects.empty())
        cout << "Objeto selecionado: " << objects[0].name << endl;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        processInput(window);

        for (SceneObject& obj : objects)
            updateTrajectory(obj, deltaTime);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));

        glActiveTexture(GL_TEXTURE0);

        for (int i = 0; i < (int)objects.size(); i++) {
            SceneObject& obj = objects[i];
            if (obj.vao == 0) continue;

            bool isSelected = (i == selectedObject);
            vec3 drawScale = isSelected ? obj.scale * 1.1f : obj.scale;

            mat4 model = mat4(1.0f);
            model = translate(model, obj.position);
            model = scale(model, drawScale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
            glUniform1f(highlightLoc, isSelected ? 1.0f : 0.0f);

            glBindTexture(GL_TEXTURE_2D, obj.texID);
            glBindVertexArray(obj.vao);
            glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void updateTrajectory(SceneObject& obj, float dt)
{
    if (obj.controlPoints.size() < 2) return;

    int next = (obj.currentPoint + 1) % (int)obj.controlPoints.size();
    vec3 from = obj.controlPoints[obj.currentPoint];
    vec3 to   = obj.controlPoints[next];

    float segLen = length(to - from);
    if (segLen > 1e-4f)
        obj.t += obj.speed * dt / segLen;

    // Avançar para o próximo segmento quando t >= 1
    if (obj.t >= 1.0f) {
        obj.t -= 1.0f;
        obj.currentPoint = next;
        next = (obj.currentPoint + 1) % (int)obj.controlPoints.size();
    }

    obj.position = mix(obj.controlPoints[obj.currentPoint],
                       obj.controlPoints[next], obj.t);
}

bool loadScene(const string& filePath)
{
    ifstream file(filePath);
    if (!file.is_open()) return false;

    objects.clear();
    SceneObject* current = nullptr;

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "object") {
            objects.emplace_back();
            current = &objects.back();
            float sx, sy, sz, spd;
            ss >> current->name >> current->objPath >> current->texPath
               >> sx >> sy >> sz >> spd;
            current->scale = vec3(sx, sy, sz);
            current->speed = spd;
        } else if (token == "point" && current != nullptr) {
            float x, y, z;
            ss >> x >> y >> z;
            current->controlPoints.push_back(vec3(x, y, z));
            if (current->controlPoints.size() == 1)
                current->position = current->controlPoints[0];
        }
    }

    return !objects.empty();
}

void saveScene(const string& filePath)
{
    ofstream file(filePath);
    if (!file.is_open()) {
        cout << "ERRO: não foi possível salvar em " << filePath << endl;
        return;
    }

    file << "# Trajetorias - arquivo de configuração da cena\n";
    file << "# Formato: object nome obj_path tex_path sx sy sz speed\n";
    file << "#          point x y z\n\n";

    for (const SceneObject& obj : objects) {
        file << "object " << obj.name << " "
             << obj.objPath << " " << obj.texPath << " "
             << obj.scale.x << " " << obj.scale.y << " " << obj.scale.z << " "
             << obj.speed << "\n";
        for (const vec3& p : obj.controlPoints)
            file << "point " << p.x << " " << p.y << " " << p.z << "\n";
        file << "\n";
    }

    cout << "Trajetórias salvas em " << filePath << endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS && !objects.empty()) {
        selectedObject = (selectedObject + 1) % (int)objects.size();
        cout << "Objeto selecionado: [" << selectedObject << "] "
             << objects[selectedObject].name
             << " (" << objects[selectedObject].controlPoints.size() << " pontos)" << endl;
    }

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && action == GLFW_PRESS) {
        int idx = key - GLFW_KEY_1;
        if (idx < (int)objects.size()) {
            selectedObject = idx;
            cout << "Objeto selecionado: [" << selectedObject << "] "
                 << objects[selectedObject].name
                 << " (" << objects[selectedObject].controlPoints.size() << " pontos)" << endl;
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS && !objects.empty()) {
        SceneObject& obj = objects[selectedObject];
        vec3 pt = camera.position;
        obj.controlPoints.push_back(pt);
        cout << "Ponto adicionado ao objeto '" << obj.name << "': ("
             << pt.x << ", " << pt.y << ", " << pt.z << ")"
             << " — total: " << obj.controlPoints.size() << " pontos" << endl;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
        saveScene("../assets/trajetorias.txt");
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
    float yOffset =  lastY - (float)ypos;

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

int loadSimpleOBJ(const string& filePath, int& nVertices)
{
    vector<vec3> positions;
    vector<vec2> texCoords;
    vector<vec3> normals;
    vector<GLfloat> vBuffer;

    ifstream arq(filePath.c_str());
    if (!arq.is_open()) {
        cerr << "Erro ao abrir: " << filePath << endl;
        return 0;
    }

    string line;
    while (getline(arq, line)) {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "v") {
            vec3 v; ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        } else if (word == "vt") {
            vec2 vt; ss >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } else if (word == "vn") {
            vec3 vn; ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        } else if (word == "f") {
            while (ss >> word) {
                int vi = 0, ti = 0, ni = 0;
                istringstream fs(word);
                string idx;
                if (getline(fs, idx, '/')) vi = !idx.empty() ? stoi(idx) - 1 : 0;
                if (getline(fs, idx, '/')) ti = !idx.empty() ? stoi(idx) - 1 : 0;
                if (getline(fs, idx))      ni = !idx.empty() ? stoi(idx) - 1 : 0;

                vBuffer.push_back(positions[vi].x);
                vBuffer.push_back(positions[vi].y);
                vBuffer.push_back(positions[vi].z);

                if (!texCoords.empty()) {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
            }
        }
    }
    arq.close();

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

    nVertices = (int)(vBuffer.size() / 5);
    cout << "OBJ carregado: " << filePath << " (" << nVertices << " vértices)" << endl;
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

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        cout << "Textura carregada: " << filePath << endl;
    } else {
        cout << "Falha ao carregar textura: " << filePath << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}