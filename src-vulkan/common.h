#pragma once

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

#define ASSERT(cond)                                                                                                   \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(cond))                                                                                                   \
		{                                                                                                              \
			char buffer[256];                                                                                          \
			snprintf(buffer, sizeof(buffer), "Assertion failed: %s at %s:%d", #cond, __FILE__, __LINE__);              \
			throw buffer;                                                                                              \
		}                                                                                                              \
	} while (0)

#define VK_ASSERT(call)                                                                                                \
	do                                                                                                                 \
	{                                                                                                                  \
		VkResult result_ = call;                                                                                       \
		if (result_ != VK_SUCCESS)                                                                                     \
		{                                                                                                              \
			char buffer[256];                                                                                          \
			snprintf(buffer, sizeof(buffer), "Vulkan error: %d at %s:%d", result_, __FILE__, __LINE__);                \
			throw buffer;                                                                                              \
		}                                                                                                              \
	} while (0)

typedef unsigned char	   u8;
typedef unsigned short	   u16;
typedef unsigned int	   u32;
typedef unsigned long long u64;

typedef char	  i8;
typedef short	  i16;
typedef int		  i32;
typedef long long i64;

typedef float  f32;
typedef double f64;

typedef bool b8;

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on