#pragma once
#include <cstdio>
#include <signal.h>

#define ASSERT(x)                                                                                                      \
	if (!(x))                                                                                                          \
	{                                                                                                                  \
		fprintf(stderr, "Assertion Failed: %s, file %s, line %d\n", #x, __FILE__, __LINE__);                           \
		__builtin_trap();                                                                                              \
	}

#define _STRINGIFY(x) #x
#define STRINGIFY(x)  _STRINGIFY(x)