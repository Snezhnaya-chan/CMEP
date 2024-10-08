cmake_minimum_required(VERSION 3.10)

# Also build glfw
add_subdirectory(glfw)

# This script is a mess
# but sadly LuaJIT does not have a cmake compatible build system
# We use the author's native build system (make for unix-like, msvcbuild.bat for windows)

# Debug info
message("-- [EXTERNAL] Source dir is: " ${CMAKE_CURRENT_SOURCE_DIR})

# Setup various platform-specific semantics
if(NOT MSVC) # Unix-like
	message("-- Will compile luajit for unix-like")

	set(VAR_LUAJIT_NAME libluajit)
	set(VAR_STATICLIB_EXTENSION a)
	set(VAR_DYLIB_EXTENSION so)
	
	set(VAR_SYMLINK_FILENAME ${VAR_LUAJIT_NAME}-5.1.${VAR_DYLIB_EXTENSION}.2)

	if(NOT APPLE) # Probably Non-OSX Unix-like
		set(VAR_BUILD_COMMAND make)
	else() # Probably OSX
		set(VAR_OSX_DEPLOYMENT_TARGET 10.14)
		set(VAR_BUILD_COMMAND MACOSX_DEPLOYMENT_TARGET=${VAR_OSX_DEPLOYMENT_TARGET} make)
	endif()

else() # Windows
	message("-- Will compile luajit for Windows")

	set(VAR_LUAJIT_NAME lua51)
	set(VAR_DYLIB_EXTENSION dll)
	set(VAR_STATICLIB_EXTENSION lib)

	set(VAR_BUILD_COMMAND msvcbuild.bat)

endif()

set(VAR_OUTPUT_DIRECTORY ${VAR_BUILD_DIRECTORY})
set(VAR_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/luajit/src)

set(VAR_DYLIB_FILENAME ${VAR_LUAJIT_NAME}.${VAR_DYLIB_EXTENSION})
set(VAR_STATICLIB_FILENAME ${VAR_LUAJIT_NAME}.${VAR_STATICLIB_EXTENSION})

# These 2 files have to be ALWAYS generated by this script
set(VAR_FINAL_FILENAMES 
	${VAR_DYLIB_FILENAME}
	${VAR_STATICLIB_FILENAME}
	)

set(VAR_FINAL_OUTPUT)

if(NOT MSVC AND NOT APPLE) # Non-OSX Unix-like specifics

	# Add the name of the symlink to the list of generated files
	list(APPEND VAR_FINAL_FILENAMES ${VAR_SYMLINK_FILENAME})

	# Create necessary linux symlink
	add_custom_command(OUTPUT ${VAR_OUTPUT_DIRECTORY}/${VAR_SYMLINK_FILENAME}

			   		   DEPENDS ${VAR_OUTPUT_DIRECTORY}/${VAR_DYLIB_FILENAME}

					   COMMAND ${CMAKE_COMMAND} -E create_symlink ./${VAR_DYLIB_FILENAME} ./${VAR_SYMLINK_FILENAME}

					   WORKING_DIRECTORY ${VAR_OUTPUT_DIRECTORY}
					   )

endif()

# Convert every filename that this script generates to an absolute path in the build directory
foreach(FILENAME ${VAR_FINAL_FILENAMES})
	list(APPEND VAR_FINAL_OUTPUT ${VAR_OUTPUT_DIRECTORY}/${FILENAME})
endforeach()

# Create a custom target depending on all the output files
add_custom_target(lua_build
				  DEPENDS ${VAR_FINAL_OUTPUT}
				  WORKING_DIRECTORY ${VAR_OUTPUT_DIRECTORY})

# Build luajit using platform specific command
# Creates a dynamic library and a static library
add_custom_command(OUTPUT
				   ${VAR_SOURCE_DIRECTORY}/${VAR_DYLIB_FILENAME}
				   ${VAR_SOURCE_DIRECTORY}/${VAR_STATICLIB_FILENAME}

				   COMMAND ${VAR_BUILD_COMMAND}

				   WORKING_DIRECTORY ${VAR_SOURCE_DIRECTORY}/)

# Copy luajit static and dynamic library
# from the source directory
# to the output directory
add_custom_command(OUTPUT
				   ${VAR_OUTPUT_DIRECTORY}/${VAR_DYLIB_FILENAME}
				   ${VAR_OUTPUT_DIRECTORY}/${VAR_STATICLIB_FILENAME}

				   DEPENDS
				   ${VAR_SOURCE_DIRECTORY}/${VAR_DYLIB_FILENAME}
				   ${VAR_SOURCE_DIRECTORY}/${VAR_STATICLIB_FILENAME}

				   COMMAND ${CMAKE_COMMAND} -E copy
				   # From
				   ./${VAR_DYLIB_FILENAME}
				   ./${VAR_STATICLIB_FILENAME}
				   # To
				   ${VAR_OUTPUT_DIRECTORY}/

				   WORKING_DIRECTORY ${VAR_SOURCE_DIRECTORY}/)
	