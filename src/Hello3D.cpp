/* Hello 3D - Cubo
 *
 * Tarefa 1 (implementação): a partir do projeto base da pirâmide,
 *   - a geometria foi transformada em um CUBO (6 faces, cada uma com cor
 *     diferente, formadas por 2 triângulos cada -> 12 triângulos / 36 vértices);
 *   - controle por teclado para TRANSLADAR o cubo nos 3 eixos;
 *   - controle por teclado para ESCALA UNIFORME;
 *   - possibilidade de INSTANCIAR mais de um cubo na cena.
 *
 * Base adaptada de https://learnopengl.com/#!Getting-started/Hello-Triangle
 * por Rossana Baptista Queiroz (Processamento Gráfico/Computação Gráfica - Unisinos).
 */

#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Processa as teclas que ficam pressionadas (translação / escala contínuas)
void processInput(GLFWwindow* window, float deltaTime);

int setupShader();
int setupGeometry();

const GLuint WIDTH = 1000, HEIGHT = 1000;
const int CUBE_VERTICES = 36;
const GLchar* vertexShaderSource = "#version 450\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = projection * view * model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 450\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

// Estado de rotação automática (ligado/desligado por eixo via teclas X/Y/Z)
bool rotateX = false, rotateY = false, rotateZ = false;

vector<glm::vec3> cubes;
glm::vec3 userTranslate = glm::vec3(0.0f);
float     userScale     = 1.0f;

int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Cubo!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Gerando um buffer com a geometria do cubo
	GLuint VAO = setupGeometry();

	glUseProgram(shaderID);

	GLint modelLoc      = glGetUniformLocation(shaderID, "model");
	GLint viewLoc       = glGetUniformLocation(shaderID, "view");
	GLint projectionLoc = glGetUniformLocation(shaderID, "projection");

	// Matriz de projeção (perspectiva) e de visualização (câmera fixa)
	glm::mat4 projection = glm::perspective(glm::radians(45.0f),
		(float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = glm::lookAt(
		glm::vec3(0.0f, 1.5f, 6.0f),  // posição da câmera
		glm::vec3(0.0f, 0.0f, 0.0f),  // ponto observado
		glm::vec3(0.0f, 1.0f, 0.0f)); // vetor "up"

	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	glEnable(GL_DEPTH_TEST);

	cubes.push_back(glm::vec3(-2.0f, 0.0f, 0.0f));
	cubes.push_back(glm::vec3( 0.0f, 0.0f, 0.0f));
	cubes.push_back(glm::vec3( 2.0f, 0.0f, 0.0f));

	cout << "\n========== CONTROLES ==========\n"
	     << " Rotacao:    X / Y / Z  (liga-desliga giro no eixo)\n"
	     << " Transladar: A/D = eixo X   W/S = eixo Z   I/J = eixo Y\n"
	     << " Escala:     [ diminui   ] aumenta  (uniforme)\n"
	     << " Cubos:      N = adiciona   B = remove o ultimo\n"
	     << " Reset:      R = zera translacao/escala\n"
	     << " Sair:       ESC\n"
	     << "===============================\n" << endl;

	float lastFrame = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = (float)glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		processInput(window, deltaTime);

		glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float angle = currentFrame;

		glBindVertexArray(VAO);

		// Desenha cada cubo instanciado na cena
		for (const glm::vec3& basePos : cubes)
		{
			glm::mat4 model = glm::mat4(1.0f);

			// 1) Translação: posição-base do cubo + deslocamento do usuário
			model = glm::translate(model, basePos + userTranslate);

			// 2) Rotação automática nos eixos habilitados
			if (rotateX) model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
			if (rotateY) model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
			if (rotateZ) model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

			// 3) Escala uniforme controlada pelo usuário
			model = glm::scale(model, glm::vec3(userScale));

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_TRIANGLES, 0, CUBE_VERTICES);
		}

		glBindVertexArray(0);
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glfwTerminate();
	return 0;
}

// Teclas SEGURADAS: translação e escala contínuas (independentes do frame rate)
void processInput(GLFWwindow* window, float deltaTime)
{
	float transSpeed = 3.0f * deltaTime; // unidades por segundo
	float scaleSpeed = 1.5f * deltaTime;

	// Translação no eixo X (A / D) e no eixo Z (W / S)
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) userTranslate.x -= transSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) userTranslate.x += transSpeed;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) userTranslate.z -= transSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) userTranslate.z += transSpeed;

	// Translação no eixo Y (I sobe / J desce)
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) userTranslate.y += transSpeed;
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) userTranslate.y -= transSpeed;

	// Escala uniforme ( [ diminui  ] aumenta ), com limite mínimo
	if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
		userScale = glm::max(0.1f, userScale - scaleSpeed);
	if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
		userScale += scaleSpeed;
}

// Função de callback de teclado - teclas PRESSIONADAS uma vez (ações discretas)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (action != GLFW_PRESS) return;

	if (key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Liga/desliga a rotação automática em cada eixo
	if (key == GLFW_KEY_X) rotateX = !rotateX;
	if (key == GLFW_KEY_Y) rotateY = !rotateY;
	if (key == GLFW_KEY_Z) rotateZ = !rotateZ;

	// Instancia um novo cubo em uma posição deslocada do último
	if (key == GLFW_KEY_N)
	{
		glm::vec3 pos = cubes.empty() ? glm::vec3(0.0f)
		                              : cubes.back() + glm::vec3(0.0f, 1.5f, -1.5f);
		cubes.push_back(pos);
		cout << "Cubos na cena: " << cubes.size() << endl;
	}

	// Remove o último cubo instanciado (mantém ao menos 1)
	if (key == GLFW_KEY_B && cubes.size() > 1)
	{
		cubes.pop_back();
		cout << "Cubos na cena: " << cubes.size() << endl;
	}

	// Reset das transformações do usuário
	if (key == GLFW_KEY_R)
	{
		userTranslate = glm::vec3(0.0f);
		userScale = 1.0f;
		cout << "Translacao e escala resetadas." << endl;
	}
}

int setupGeometry()
{
	GLfloat vertices[] = {

		// Face FRENTE (z = +0.5) - vermelho
		-0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,

		// Face TRÁS (z = -0.5) - verde
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,

		// Face ESQUERDA (x = -0.5) - azul
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,

		// Face DIREITA (x = +0.5) - amarelo
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,

		// Face TOPO (y = +0.5) - magenta
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,

		// Face BASE (y = -0.5) - ciano
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
	};

	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}
