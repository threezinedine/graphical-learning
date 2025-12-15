#include "common.h"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <easy/profiler.h>
#include <fstream>
#include <string>

std::string readFile(const std::string& filepath);

int main()
{
	EASY_PROFILER_ENABLE;
	profiler::startListen();

	ASSERT(glfwInit() == GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Window", nullptr, nullptr);
	ASSERT(window != nullptr);

	glfwMakeContextCurrent(window);
	ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));

	int			   width, height, channels;
	unsigned char* data =
		stbi_load(STRINGIFY(SOURCE_DIR) "/assets/images/meed-logo.png", &width, &height, &channels, 0);
	ASSERT(data != nullptr);

	printf("Loaded image with width: %d, height: %d, channels: %d\n", width, height, channels);

	stbi_image_free(data);

	std::string vertexShaderSource = readFile(STRINGIFY(SOURCE_DIR) "/assets/shaders/opengl/simple.vert");
	ASSERT(!vertexShaderSource.empty());
	GLuint		vertexShader	 = glCreateShader(GL_VERTEX_SHADER);
	const char* vertexSourceCStr = vertexShaderSource.c_str();
	GL_ASSERT(glShaderSource(vertexShader, 1, &vertexSourceCStr, nullptr));
	GL_ASSERT(glCompileShader(vertexShader));

	bool success;
	GL_ASSERT(glGetShaderiv(vertexShader, GL_COMPILE_STATUS, (int*)&success));
	if (!success)
	{
		char infoLog[512];
		GL_ASSERT(glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog));
		fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
		ASSERT(false);
	}

	std::string fragmentShaderSource = readFile(STRINGIFY(SOURCE_DIR) "/assets/shaders/opengl/simple.frag");
	ASSERT(!fragmentShaderSource.empty());
	GLuint		fragmentShader	   = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fragmentSourceCStr = fragmentShaderSource.c_str();
	GL_ASSERT(glShaderSource(fragmentShader, 1, &fragmentSourceCStr, nullptr));
	GL_ASSERT(glCompileShader(fragmentShader));

	GL_ASSERT(glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, (int*)&success));
	if (!success)
	{
		char infoLog[512];
		GL_ASSERT(glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog));
		fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", infoLog);
		ASSERT(false);
	}

	GLuint shaderProgram = glCreateProgram();
	GL_ASSERT(glAttachShader(shaderProgram, vertexShader));
	GL_ASSERT(glAttachShader(shaderProgram, fragmentShader));
	GL_ASSERT(glLinkProgram(shaderProgram));

	GL_ASSERT(glGetProgramiv(shaderProgram, GL_LINK_STATUS, (int*)&success));
	if (!success)
	{
		char infoLog[512];
		GL_ASSERT(glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog));
		fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s", infoLog);
		ASSERT(false);
	}

	GL_ASSERT(glDeleteShader(vertexShader));
	GL_ASSERT(glDeleteShader(fragmentShader));

	while (!glfwWindowShouldClose(window))
	{
		EASY_BLOCK("Main Loop");
		glClear(GL_COLOR_BUFFER_BIT);

		GL_ASSERT(glUseProgram(shaderProgram));
		GL_ASSERT(glDrawArrays(GL_TRIANGLES, 0, 3));

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	profiler::dumpBlocksToFile(STRINGIFY(SOURCE_DIR) "/logs/log.prof");
	return 0;
}

std::string readFile(const std::string& filepath)
{
	std::ifstream fileStream(filepath, std::ios::in);
	ASSERT(fileStream.is_open());

	std::string content;
	std::string line;
	while (std::getline(fileStream, line))
	{
		content += line + "\n";
	}

	fileStream.close();
	return content;
}