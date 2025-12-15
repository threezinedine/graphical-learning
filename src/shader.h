#pragma once
#include "common.h"

namespace ntt {

enum ShaderType
{
	VERTEX_SHADER,
	FRAGMENT_SHADER,
	GEOMETRY_SHADER,
	TESS_CONTROL_SHADER,
	TESS_EVALUATION_SHADER,
};

class Shader
{
public:
	Shader(const std::string& filePath, ShaderType type);
	Shader(const Shader& other) = delete;
	Shader(Shader&& other) noexcept;
	~Shader();

public:
	inline u32 getId() const
	{
		return m_shaderId;
	}

	inline ShaderType getType() const
	{
		return m_type;
	}

private:
	ShaderType m_type;
	u32		   m_shaderId;
};

} // namespace ntt