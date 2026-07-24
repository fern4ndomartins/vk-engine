cmake_minimum_required(VERSION 3.10)
project(main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)



include_directories(include)
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cc")

find_program(GLSLC_EXECUTABLE glslc HINTS "$ENV{VULKAN_SDK}/bin")
file(GLOB_RECURSE GLSL_SHADERS
    "${CMAKE_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/shaders/*.comp"
)
set(SPIRV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
foreach(GLSL ${GLSL_SHADERS})
    get_filename_component(EXTENSION ${GLSL} EXT)     
    string(REPLACE "." "" EXTENSION ${EXTENSION})    
    set(SPIRV "${SPIRV_OUTPUT_DIR}/${EXTENSION}.spv") 

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIRV_OUTPUT_DIR}
        COMMAND ${GLSLC_EXECUTABLE} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
        COMMENT "Compiling shader: ${FILE_NAME}"
        VERBATIM
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()
add_custom_target(Shaders ALL DEPENDS ${SPIRV_BINARY_FILES})    

add_executable(main ${SOURCES})
add_dependencies(main Shaders)

target_link_libraries(main Vulkan::Vulkan glfw)