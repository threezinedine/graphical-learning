#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "texture.h"

namespace ntt {

Texture::Texture(const std::string& path)
	: m_textureId(0)
	, m_unit(-1)
{
	i32 width, height, channels;

	u8* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
	ASSERT(data != nullptr);

	GL_ASSERT(glCreateTextures(GL_TEXTURE_2D, 1, &m_textureId));

	switch (channels)
	{
	case 3:
		GL_ASSERT(glTextureStorage2D(m_textureId, 1, GL_RGB8, width, height));
		GL_ASSERT(glTextureSubImage2D(m_textureId, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data));
		break;
	case 4:
		GL_ASSERT(glTextureStorage2D(m_textureId, 1, GL_RGBA8, width, height));
		GL_ASSERT(glTextureSubImage2D(m_textureId, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data));
		break;
	default:
		ASSERT(false && "Unsupported number of channels in texture");
	}

	GL_ASSERT(glTextureParameteri(m_textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GL_ASSERT(glTextureParameteri(m_textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_ASSERT(glTextureParameteri(m_textureId, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL_ASSERT(glTextureParameteri(m_textureId, GL_TEXTURE_WRAP_T, GL_REPEAT));

	stbi_image_free(data);
}

Texture::Texture(Texture&& other) noexcept
	: m_textureId(other.m_textureId)
	, m_unit(other.m_unit)
{
	other.m_textureId = 0;
	other.m_unit	  = -1;
}

Texture::~Texture()
{
	if (m_textureId != 0)
	{
		GL_ASSERT(glDeleteTextures(1, &m_textureId));
		m_textureId = 0;
	}
}

void Texture::bind(GLuint unit)
{
	m_unit = static_cast<i32>(unit);
	GL_ASSERT(glActiveTexture(GL_TEXTURE0 + unit));
	GL_ASSERT(glBindTexture(GL_TEXTURE_2D, m_textureId));
}

void Texture::unbind()
{
	ASSERT(m_unit != -1);

	GL_ASSERT(glActiveTexture(GL_TEXTURE0 + m_unit));
	GL_ASSERT(glBindTexture(GL_TEXTURE_2D, 0));
	m_unit = -1;
}

} // namespace ntt
