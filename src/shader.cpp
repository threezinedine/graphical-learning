#include "shader.h"
#include "utils.h"

namespace ntt {

Shader::Shader(const std::string& filePath, ShaderType type)
	: m_type(type)
{
	std::string shaderSource = readFile(filePath);
	ASSERT(!shaderSource.empty());

	GLenum shaderTypeGL;

	switch (type)
	{
	case VERTEX_SHADER:
		shaderTypeGL = GL_VERTEX_SHADER;
		break;
	case FRAGMENT_SHADER:
		shaderTypeGL = GL_FRAGMENT_SHADER;
		break;
	case GEOMETRY_SHADER:
		shaderTypeGL = GL_GEOMETRY_SHADER;
		break;
	case TESS_CONTROL_SHADER:
		shaderTypeGL = GL_TESS_CONTROL_SHADER;
		break;
	case TESS_EVALUATION_SHADER:
		shaderTypeGL = GL_TESS_EVALUATION_SHADER;
		break;
	default:
		ASSERT(false); // Unknown shader type
	}

	m_shaderId			   = glCreateShader(shaderTypeGL);
	const char* sourceCStr = shaderSource.c_str();
	GL_ASSERT(glShaderSource(m_shaderId, 1, &sourceCStr, nullptr));
	GL_ASSERT(glCompileShader(m_shaderId));

	bool success;
	GL_ASSERT(glGetShaderiv(m_shaderId, GL_COMPILE_STATUS, (int*)&success));
	if (!success)
	{
		char infoLog[512];
		GL_ASSERT(glGetShaderInfoLog(m_shaderId, 512, nullptr, infoLog));
		fprintf(stderr, "ERROR::SHADER::COMPILATION_FAILED\n%s", infoLog);
		ASSERT(false);
	}
}

Shader::Shader(Shader&& other) noexcept
	: m_type(other.m_type)
	, m_shaderId(other.m_shaderId)
{
	other.m_shaderId = 0; // Invalidate the moved-from object
}

Shader::~Shader()
{
	if (m_shaderId != 0)
	{
		GL_ASSERT(glDeleteShader(m_shaderId));
		m_shaderId = 0;
	}
}

} // namespace ntt