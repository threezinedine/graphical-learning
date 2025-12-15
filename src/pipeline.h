#pragma once

#include "common.h"
#include "shader.h"

namespace ntt {

class Pipeline
{
public:
	Pipeline(Shader* shaders, u32 shaderCount);
	Pipeline(const Pipeline& other) = delete;
	Pipeline(Pipeline&& other) noexcept;
	~Pipeline();

public:
	inline u32 getProgramId() const
	{
		return m_programId;
	}

	inline void bind() const
	{
		GL_ASSERT(glUseProgram(m_programId));
	}

	inline void unbind() const
	{
		GL_ASSERT(glUseProgram(0));
	}

private:
	u32 m_programId;
};

} // namespace ntt