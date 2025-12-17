#include "vertex_buffer.h"

namespace ntt {

static u32 getAttributeTypeBytes(VertexAttributeType type)
{
	switch (type)
	{
	case VertexAttributeType::FLOAT:
		return 1;
	case VertexAttributeType::VEC2:
		return 2;
	case VertexAttributeType::VEC3:
		return 3;
	case VertexAttributeType::VEC4:
		return 4;
	default:
		ASSERT(false && "Unknown VertexAttributeType");
		return 0;
	}
}

VertexBuffer::VertexBuffer(const std::initializer_list<VertexAttributeType>& attributeTypes)
{
	GL_ASSERT(glGenBuffers(1, &m_vbo));

	GL_ASSERT(glGenVertexArrays(1, &m_vao));
	GL_ASSERT(glBindVertexArray(m_vao));
	GL_ASSERT(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));

	u32 attributesCount = u32(attributeTypes.size());
	m_attributes.resize(attributesCount);

	u32 bytes = 0;
	for (const auto& type : attributeTypes)
	{
		bytes += getAttributeTypeBytes(type);
	}

	u32 offset = 0;
	for (u32 i = 0; i < attributesCount; ++i)
	{
		VertexAttributeType type = *(std::next(attributeTypes.begin(), i));
		VertexAttribute&	attr = m_attributes[i];
		attr.size				 = getAttributeTypeBytes(type);

		GL_ASSERT(glEnableVertexAttribArray(i));
		GL_ASSERT(glVertexAttribPointer(
			i, attr.size, GL_FLOAT, GL_FALSE, bytes * sizeof(float), (void*)(uintptr_t)(offset * sizeof(float))));
		offset += attr.size;
	}

	unbind();
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
	: m_attributes(other.m_attributes)
	, m_vao(other.m_vao)
	, m_vbo(other.m_vbo)
{
	other.m_vao = 0;
	other.m_vbo = 0;
}

VertexBuffer::~VertexBuffer()
{
	if (m_vbo != 0)
	{
		GL_ASSERT(glDeleteBuffers(1, &m_vbo));
		m_vbo = 0;
	}

	if (m_vao != 0)
	{
		GL_ASSERT(glDeleteVertexArrays(1, &m_vao));
		m_vao = 0;
	}
}

void VertexBuffer::update(const void* data, u32 size)
{
	bind();
	GL_ASSERT(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
	unbind();
}

} // namespace ntt
