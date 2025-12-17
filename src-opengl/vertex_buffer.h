#pragma once
#include "common.h"
#include <vector>

namespace ntt {

enum class VertexAttributeType
{
	FLOAT,
	VEC2,
	VEC3,
	VEC4,
};

struct VertexAttribute
{
	u32 size;
};

/**
 * @example
 * ```c++
 * VertexBuffer vertexBuffer(VEC2, VEC3, VEC2);
 * vertexBuffer.update(data, dataSize);
 * vertexBuffer.bind();
 * ```
 */

class VertexBuffer
{
public:
	VertexBuffer(const std::initializer_list<VertexAttributeType>& attributeTypes);
	VertexBuffer(const VertexBuffer&) = delete;
	VertexBuffer(VertexBuffer&&) noexcept;
	~VertexBuffer();

public:
	inline void bind() const
	{
		GL_ASSERT(glBindVertexArray(m_vao));
		GL_ASSERT(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
	}

	inline void unbind() const
	{
		GL_ASSERT(glBindVertexArray(0));
		GL_ASSERT(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}

	void update(const void* data, u32 size);

private:
	std::vector<VertexAttribute> m_attributes;
	u32							 m_vao;
	u32							 m_vbo;
};

}; // namespace ntt