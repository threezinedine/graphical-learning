#pragma once
#include <cstdio>
#include <signal.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;

typedef signed char	 i8;
typedef signed short i16;
typedef signed int	 i32;
typedef signed long	 i64;

typedef float  f32;
typedef double f64;

#define ASSERT(x)                                                                                                      \
	if (!(x))                                                                                                          \
	{                                                                                                                  \
		fprintf(stderr, "Assertion Failed: %s, file %s, line %d\n", #x, __FILE__, __LINE__);                           \
		__builtin_trap();                                                                                              \
	}

#define _STRINGIFY(x) #x
#define STRINGIFY(x)  _STRINGIFY(x)

#define GL_ASSERT(call)                                                                                                \
	do                                                                                                                 \
	{                                                                                                                  \
		while (glGetError() != GL_NO_ERROR);                                                                           \
		call;                                                                                                          \
		GLenum err = glGetError();                                                                                     \
		if (err != GL_NO_ERROR)                                                                                        \
			printf("GL_ASSERT: %s returned %d, file %s, line %d\n", #call, err, __FILE__, __LINE__);                   \
		ASSERT(err == GL_NO_ERROR);                                                                                    \
	} while (0)

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <string>

#include <glm/glm.hpp>
typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;