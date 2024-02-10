cmake_minimum_required(VERSION "3.22.1")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

add_custom_target(shaders)
set(GLSLC "/usr/bin/glslc")

file(GLOB_RECURSE SHADERS_SRC "../Shaders/*.vert" "../Shaders/*.frag" "../Shaders/*.comp")
foreach(FILE ${SHADERS_SRC})
    get_filename_component(FILE_NAME ${FILE} NAME)
    set(COMPILED_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FILE_NAME}.spv")
    if (${FILE} IS_NEWER_THAN ${COMPILED_FILE})
        add_custom_command( TARGET shaders
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                            DEPENDS ${FILE}
                            COMMAND ${GLSLC} --target-env=vulkan1.3 ${FILE} -o ${COMPILED_FILE}
                            COMMENT "Compiling ${FILE_NAME}"
        )
    endif()
endforeach(FILE)



project(vulkano)
project(debugVulkano)

file(GLOB_RECURSE SRC "../vulkano/Backend/src/*.cpp")
add_executable(vulkano "../main.cpp" "${SRC}")
add_executable(debugVulkano "../main.cpp" "${SRC}")
add_dependencies(vulkano shaders)
add_dependencies(debugVulkano shaders)
target_compile_options(vulkano PRIVATE "-Wall")
target_compile_options(debugVulkano PRIVATE "-Wall" "-g")

find_package(Vulkan REQUIRED)
target_link_libraries(vulkano ${Vulkan_LIBRARIES})
target_link_libraries(debugVulkano ${Vulkan_LIBRARIES})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("../3rdParty/GLFW" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(vulkano glfw)
target_link_libraries(debugVulkano glfw)

set(IMGUI_PATH "../3rdParty/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp) 
add_library("ImGui" STATIC ${IMGUI_SOURCES})
target_include_directories("ImGui" PUBLIC ${IMGUI_PATH})
target_link_libraries(vulkano ImGui)
target_link_libraries(debugVulkano ImGui)

target_include_directories(vulkano PUBLIC "3rdParty" "../vulkano/" "../vulkano/Backend/Headers/")
target_include_directories(debugVulkano PUBLIC "3rdParty" "../vulkano/" "../vulkano/Backend/Headers/")
