#include "common.h"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <easy/profiler.h>
#include <fstream>
#include <string>

#include "pipeline.h"
#include "shader.h"
#include "texture.h"
#include "vertex_buffer.h"

using namespace ntt;

struct Vertex
{
	vec2 position;
	vec3 color;
	vec2 texCoord;
};

// clang-format off
const Vertex vertices[] = {
	{{ 0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // Top vertex (Red)
	{{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}}, // Bottom-left vertex (Green)
	{{ 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // Bottom-right vertex (Blue)
};
// clang-format on

int main()
{
	EASY_PROFILER_ENABLE;
	profiler::startListen();

	ASSERT(glfwInit() == GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Window", nullptr, nullptr);
	ASSERT(window != nullptr);

	glfwMakeContextCurrent(window);
	ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));

	Texture texture(STRINGIFY(SOURCE_DIR) "/assets/images/meed-logo.png");

	Shader vertexShader(STRINGIFY(SOURCE_DIR) "/assets/shaders/opengl/simple.vert", VERTEX_SHADER);
	Shader fragmentShader(STRINGIFY(SOURCE_DIR) "/assets/shaders/opengl/simple.frag", FRAGMENT_SHADER);

	Shader shaders[2] = {std::move(vertexShader), std::move(fragmentShader)};

	Pipeline	 pipeline(shaders, 2);
	VertexBuffer buffer({VertexAttributeType::VEC2, VertexAttributeType::VEC3, VertexAttributeType::VEC2});

	buffer.update(vertices, sizeof(vertices));

	while (!glfwWindowShouldClose(window))
	{
		EASY_BLOCK("Main Loop");
		glClear(GL_COLOR_BUFFER_BIT);

		buffer.bind();
		pipeline.bind();
		texture.bind(0);
		GL_ASSERT(glDrawArrays(GL_TRIANGLES, 0, 3));
		texture.unbind();
		pipeline.unbind();
		buffer.unbind();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	buffer.~VertexBuffer();
	pipeline.~Pipeline();
	texture.~Texture();

	glfwDestroyWindow(window);
	glfwTerminate();
	profiler::dumpBlocksToFile(STRINGIFY(SOURCE_DIR) "/logs/log.prof");
	return 0;
}
