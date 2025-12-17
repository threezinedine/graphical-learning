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

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/version.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace ntt;

struct Vertex
{
	vec2 position;
	vec3 color;
	vec2 texCoord;
};

struct VertexData
{
	vec3 position;
	vec2 texCoord;
};

struct UniformBufferObject
{
	glm::mat4 mvp;
};

// clang-format off
const Vertex vertices[] = {
	{{ 0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // Top vertex (Red)
	{{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}}, // Bottom-left vertex (Green)
	{{ 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // Bottom-right vertex (Blue)
};
// clang-format on

#define WIDTH  800
#define HEIGHT 600

int main()
{
	EASY_PROFILER_ENABLE;
	profiler::startListen();

	ASSERT(glfwInit() == GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "GLFW Window", nullptr, nullptr);
	ASSERT(window != nullptr);

	glfwMakeContextCurrent(window);
	ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	Texture texture(STRINGIFY(SOURCE_DIR) "/assets/images/meed-logo.png");

	Shader vertexShader(STRINGIFY(SOURCE_DIR) "/assets/shaders/opengl/simple.vert", VERTEX_SHADER);
	Shader fragmentShader(STRINGIFY(SOURCE_DIR) "/assets/shaders/opengl/simple.frag", FRAGMENT_SHADER);
	Shader geometryShader(STRINGIFY(SOURCE_DIR) "/assets/shaders/opengl/simple.geom", GEOMETRY_SHADER);

	Shader shaders[3] = {std::move(vertexShader), std::move(fragmentShader), std::move(geometryShader)};

	Pipeline	 pipeline(shaders, sizeof(shaders) / sizeof(Shader));
	VertexBuffer buffer({VertexAttributeType::VEC2, VertexAttributeType::VEC3, VertexAttributeType::VEC2});

	const aiScene* scene = aiImportFile(STRINGIFY(SOURCE_DIR) "/assets/gltfs/rubber_duck.gltf", aiProcess_Triangulate);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		printf("ASSIMP ERROR: %s\n", "error");
		return -1;
	}

	const aiMesh* mesh = scene->mMeshes[0];

	std::vector<VertexData> verticies;
	for (u32 vIndex = 0u; vIndex < mesh->mNumVertices; ++vIndex)
	{
		aiVector3D position = mesh->mVertices[vIndex];
		aiVector3D texCoord = mesh->mTextureCoords[0][vIndex];

		verticies.push_back({vec3(position.x, position.z, position.y), vec2(texCoord.x, texCoord.y)});
	}

	u32 verticiesSize = u32(sizeof(VertexData)) * static_cast<u32>(verticies.size());

	std::vector<u32> indices;
	for (u32 fIndex = 0u; fIndex < mesh->mNumFaces; ++fIndex)
	{
		aiFace face = mesh->mFaces[fIndex];
		for (u32 iIndex = 0u; iIndex < face.mNumIndices; ++iIndex)
		{
			indices.push_back(face.mIndices[iIndex]);
		}
	}
	u32 indicesSize = u32(sizeof(u32)) * static_cast<u32>(indices.size());

	u32 verticesBuffer;
	GL_ASSERT(glGenBuffers(1, &verticesBuffer));
	GL_ASSERT(glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesBuffer));
	GL_ASSERT(glBufferData(GL_SHADER_STORAGE_BUFFER, verticiesSize, verticies.data(), GL_STATIC_DRAW));

	u32 indicesBuffer;
	GL_ASSERT(glGenBuffers(1, &indicesBuffer));
	GL_ASSERT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer));
	GL_ASSERT(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, indices.data(), GL_STATIC_DRAW));

	u32 vao;
	GL_ASSERT(glGenVertexArrays(1, &vao));
	GL_ASSERT(glBindVertexArray(vao));
	// GL_ASSERT(glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer));
	GL_ASSERT(glVertexArrayElementBuffer(vao, indicesBuffer));

	Texture duckTexture(STRINGIFY(SOURCE_DIR) "/assets/images/Duck_baseColor.png");

	// buffer.update(vertices, sizeof(vertices));

	UniformBufferObject ubo{};

	u32 mvpDataBuffer;
	GL_ASSERT(glGenBuffers(1, &mvpDataBuffer));
	GL_ASSERT(glBindBuffer(GL_UNIFORM_BUFFER, mvpDataBuffer));
	GL_ASSERT(glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBufferObject), nullptr, GL_DYNAMIC_DRAW));

	const float ratio = float(WIDTH) / float(HEIGHT);

	while (!glfwWindowShouldClose(window))
	{
		EASY_BLOCK("Main Loop");
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const glm::mat4 m = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -1.5f)),
												   (float)glfwGetTime(),
												   glm::vec3(0.0f, 1.0f, 0.0f)),
									   glm::vec3(0.5f));
		const glm::mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

		ubo.mvp = p * m;
		GL_ASSERT(glBindBuffer(GL_UNIFORM_BUFFER, mvpDataBuffer));
		GL_ASSERT(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UniformBufferObject), &ubo));

		GL_ASSERT(glBindVertexArray(vao));
		// GL_ASSERT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer));
		GL_ASSERT(glBindBufferBase(GL_UNIFORM_BUFFER, 0, mvpDataBuffer));
		GL_ASSERT(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, verticesBuffer));
		duckTexture.bind(0);
		pipeline.bind();
		GL_ASSERT(glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0));
		pipeline.unbind();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	GL_ASSERT(glDeleteBuffers(1, &verticesBuffer));
	GL_ASSERT(glDeleteBuffers(1, &indicesBuffer));
	GL_ASSERT(glDeleteVertexArrays(1, &vao));
	GL_ASSERT(glDeleteBuffers(1, &mvpDataBuffer));

	duckTexture.~Texture();
	buffer.~VertexBuffer();
	pipeline.~Pipeline();
	texture.~Texture();

	glfwDestroyWindow(window);
	glfwTerminate();
	profiler::dumpBlocksToFile(STRINGIFY(SOURCE_DIR) "/logs/log.prof");
	return 0;
}
