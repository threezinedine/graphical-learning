#pragma once

#include "common.h"

namespace ntt {

class Texture
{
public:
	Texture(const std::string& path);
	Texture(const Texture&) = delete;
	Texture(Texture&&) noexcept;
	~Texture();

public:
	void bind(GLuint unit);
	void unbind();

private:
	u32 m_textureId;
	i32 m_unit;
};

} // namespace ntt