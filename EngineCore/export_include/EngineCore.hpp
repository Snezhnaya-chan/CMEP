#pragma once

#if defined(ENGINECORE_LIBRARY_IMPLEMENTATION)
#	undef ENGINECORE_LIBRARY_IMPLEMENTATION
#endif

#define CMEP_ABI_IMPORT
#include "PlatformSemantics.hpp" // IWYU pragma: export
//
#include "../include/Exception.hpp"	   // IWYU pragma: export
#include "../include/OpaqueEngine.hpp" // IWYU pragma: export
