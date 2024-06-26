cmake_minimum_required(VERSION 3.10)
project(CMEP)

message("-- Binary dir is: " ${CMAKE_CURRENT_BINARY_DIR})
	
if(NOT MSVC)
	if(NOT APPLE)
		message("-- Will compile luajit for unix (non-osx)")
		#add_custom_target(lua_build COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/external/build_lua.sh linux)
		add_custom_target(lua_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.a ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit-5.1.so.2)	

		# Build luajit
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.a
						   COMMAND make
						   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/luajit/src)
	
		# Copy luajit to build dir
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.a
						   DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.a
						   COMMAND ${CMAKE_COMMAND} -E
						   copy ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.a ${CMAKE_CURRENT_BINARY_DIR}/../build)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit-5.1.so.2
				   DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.so
				   COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit-5.1.so.2)
	else()
		message("-- Will build luajit for MacOS (10.14 deploy target)")

		add_custom_target(lua_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.a)	

		# Build luajit
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.a
						   COMMAND MACOSX_DEPLOYMENT_TARGET=10.14 make
						   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/luajit/src)

		# Copy luajit to build dir
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/../build/libluajit.a
						   DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.a
						   COMMAND ${CMAKE_COMMAND} -E
						   copy ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.so ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/libluajit.a ${CMAKE_CURRENT_BINARY_DIR}/../build)
	endif()
else()
	message("-- Will compile luajit (Windows); Workdir is: " ${CMAKE_CURRENT_SOURCE_DIR})

	add_custom_target(lua_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../build/lua51.dll ${CMAKE_CURRENT_BINARY_DIR}/../build/lua51.lib)

	# Build luajit
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/lua51.dll ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/lua51.lib
					   COMMAND msvcbuild.bat
					   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/luajit/src)

	# Copy luajit to build dir
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../build/lua51.dll ${CMAKE_CURRENT_BINARY_DIR}/../build/lua51.lib
					   DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/lua51.dll ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/lua51.lib
					   COMMAND ${CMAKE_COMMAND} -E
					   copy ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/lua51.dll ${CMAKE_CURRENT_BINARY_DIR}/luajit/src/lua51.lib ${CMAKE_CURRENT_BINARY_DIR}/../build)

endif()
