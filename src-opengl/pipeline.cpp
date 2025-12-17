#include "pipeline.h"

namespace ntt {

Pipeline::Pipeline(Shader* shaders, u32 shaderCount)
{
	m_programId = glCreateProgram();

	for (u32 i = 0; i < shaderCount; ++i)
	{
		GL_ASSERT(glAttachShader(m_programId, shaders[i].getId()));
	}

	GL_ASSERT(glLinkProgram(m_programId));

	bool success;
	GL_ASSERT(glGetProgramiv(m_programId, GL_LINK_STATUS, (int*)&success));
	if (!success)
	{
		char infoLog[512];
		GL_ASSERT(glGetProgramInfoLog(m_programId, 512, nullptr, infoLog));
		fprintf(stderr, "ERROR::PROGRAM::LINKING_FAILED\n%s", infoLog);
		ASSERT(false);
	}

	for (u32 i = 0; i < shaderCount; ++i)
	{
		shaders[i].~Shader();
	}
}

Pipeline::Pipeline(Pipeline&& other) noexcept
	: m_programId(other.m_programId)
{
	other.m_programId = 0; // Invalidate the moved-from object
}

Pipeline::~Pipeline()
{
	if (m_programId != 0)
	{
		GL_ASSERT(glDeleteProgram(m_programId));
		m_programId = 0;
	}
}

} // namespace ntt