#include "common.h"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main()
{
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