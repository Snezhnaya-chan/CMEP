cmake_minimum_required(VERSION 3.5)
project(CMEP CXX)

set(HEADER_FILES
	../common_include/EngineCore.hpp
	)

set(SRC_FILES
	src/main.cpp
	)


if(WIN32)
	set(STATIC_LIBRARIES
		glfw3
		lua54
		)
elseif(APPLE)
	set(STATIC_LIBRARIES
		glfw
		lua
		)
else()
	set(STATIC_LIBRARIES
		glfw
		lua54
		)
endif()

add_executable(rungame ${SRC_FILES} ${HEADER_FILES})

include_directories(rungame PUBLIC ../external ../external/lua-prebuilt/include include ../common_include ../EngineCore/include ../external/VulkanMemoryAllocator/include)
if(DEFINED ENV{VULKAN_SDK})
	if(WIN32)
		target_link_directories(rungame PUBLIC $ENV{VULKAN_SDK}/Lib lib ../build/Debug ../build ../external/lua-prebuilt)
		target_include_directories(rungame PUBLIC $ENV{VULKAN_SDK}/Include)
	else()
		if(APPLE)
				target_link_directories(rungame PUBLIC $ENV{VULKAN_SDK}/lib lib ../build/Debug ../build ../external/lua-5.4.6/src)
			target_include_directories(rungame PUBLIC $ENV{VULKAN_SDK}/include)
		else()
			target_link_directories(rungame PUBLIC $ENV{VULKAN_SDK}/lib lib ../build/Debug ../build ../external/lua-5.4.6/src)
			target_include_directories(rungame PUBLIC $ENV{VULKAN_SDK}/include)
		endif()
	endif()
else()
	link_directories(rungame PUBLIC lib ../build/Debug ../build ../external/lua)
	message("Warning! Vulkan SDK environment variable not found!")
endif()


target_link_libraries(rungame EngineCore ${STATIC_LIBRARIES})