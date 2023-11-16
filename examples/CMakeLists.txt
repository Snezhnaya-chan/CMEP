cmake_minimum_required(VERSION 3.5)
project(examples)


if(MSVC)
    add_custom_target(floppybirb-compile-shaders compile.bat WORKING_DIRECTORY floppybirb\\shaders\\vulkan)
    add_custom_target(floppybirb xcopy /s /e /i /y examples\\floppybirb build\\game WORKING_DIRECTORY ..)
else()
    add_custom_target(floppybirb-compile-shaders ./compile.sh WORKING_DIRECTORY floppybirb/shaders/vulkan)
    add_custom_target(floppybirb cp -r examples/floppybirb build COMMAND mv build/floppybirb build/game WORKING_DIRECTORY ..)
endif()	

add_dependencies(floppybirb floppybirb-compile-shaders)
