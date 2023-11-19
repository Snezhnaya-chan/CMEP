cmake_minimum_required(VERSION 3.5)
project(EngineCore CXX)

set(SRC_FILES
	src/AssetManager.cpp
	src/dllmain.cpp
	src/Engine.cpp
	src/Object.cpp
	src/Scene.cpp
	src/SceneManager.cpp
	src/SceneLoader.cpp
	src/ObjectFactory.cpp
	src/Logging/ConsoleColors.cpp
	src/Logging/Logging.cpp
	src/Rendering/Font.cpp
	src/Rendering/AxisRenderer.cpp
	src/Rendering/Mesh.cpp
	src/Rendering/MeshRenderer.cpp
	src/Rendering/SpriteRenderer.cpp
	src/Rendering/TextRenderer.cpp
	src/Rendering/Texture.cpp
	src/Rendering/VulkanRenderingEngine.cpp
	src/Scripting/LuaScript.cpp
	src/Scripting/LuaScriptExecutor.cpp
	src/Scripting/Mappings.cpp
	../external/lodepng/lodepng.cpp
	)

if(WIN32)
	set(STATIC_LIBRARIES
		vulkan-1
		glfw3
		lua54
		)
elseif(APPLE)
	set(STATIC_LIBRARIES
		vulkan
		glfw
		lua
		)
else()
	set(STATIC_LIBRARIES
		vulkan
		glfw
		lua
		)
endif()

add_library(EngineCore SHARED ${SRC_FILES})

target_link_directories(EngineCore PUBLIC lib ../build/Debug ../build)
if(WIN32)
	target_link_directories(EngineCore PUBLIC ../external/lua-prebuilt)
elseif(UNIX OR APLE)
	target_link_directories(EngineCore PUBLIC ../external/lua-5.4.6/src)
endif()

target_include_directories(EngineCore PUBLIC include ../external ../external/VulkanMemoryAllocator/include ../external/lua-prebuilt/include ../common_include)
if(DEFINED ENV{VULKAN_SDK})
	if(WIN32)
		target_link_directories(EngineCore PUBLIC $ENV{VULKAN_SDK}/Lib)
		target_include_directories(EngineCore PUBLIC $ENV{VULKAN_SDK}/Include)
	else()
		if(APPLE)
			target_link_directories(EngineCore PUBLIC $ENV{VULKAN_SDK}/lib)
			target_include_directories(EngineCore PUBLIC $ENV{VULKAN_SDK}/include)
		else()
			target_link_directories(EngineCore PUBLIC $ENV{VULKAN_SDK}/lib)
			target_include_directories(EngineCore PUBLIC $ENV{VULKAN_SDK}/include)
		endif()
	endif()
else()
	message("Warning! Vulkan SDK environment variable not found!")
endif()

target_link_libraries(EngineCore ${STATIC_LIBRARIES})
