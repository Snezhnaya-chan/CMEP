cmake_minimum_required(VERSION 3.12)
project(EngineLogging CXX)

set(SRC_FILES
	src/Logging.cpp
	src/ConsoleColors.cpp
	)

add_library(EngineLogging STATIC ${SRC_FILES})

target_compile_features(EngineLogging PUBLIC cxx_std_17)
set_target_properties(EngineLogging PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(EngineLogging PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(EngineLogging PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_options(EngineLogging PRIVATE ${ENGINE_COMPILE_OPTIONS})

target_link_directories(EngineLogging PUBLIC ../build)

target_include_directories(EngineLogging PUBLIC include
											 ../external/glm
											 ../common_include
											 )
