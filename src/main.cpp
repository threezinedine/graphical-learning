#include "common.h"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

int main()
{
	ASSERT(glfwInit() == GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW Window", nullptr, nullptr);
	ASSERT(window != nullptr);

	glfwMakeContextCurrent(window);
	ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}