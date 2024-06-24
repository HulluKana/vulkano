cmake_minimum_required(VERSION "3.22.1")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

add_custom_target(shaders)

file(GLOB_RECURSE SHADERS_SRC "../Shaders/*.vert" "../Shaders/*.frag" "../Shaders/*.comp" "../Shaders/*.rgen" "../Shaders/*.rchit" "../Shaders/*.rmiss"
    "../Shaders/*.rint" "../Shaders/*.task" "../Shaders/*.mesh")
foreach(FILE ${SHADERS_SRC})
    get_filename_component(FILE_NAME ${FILE} NAME)
    set(COMPILED_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FILE_NAME}.spv")
    if (${FILE} IS_NEWER_THAN ${COMPILED_FILE})
        add_custom_command( TARGET shaders
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                            DEPENDS ${FILE}
                            COMMAND glslc --target-env=vulkan1.3 ${FILE} -o ${COMPILED_FILE}
                            COMMENT "Compiling ${FILE_NAME}"
        )
    endif()
endforeach(FILE)



project(vulkanoManySpheres)
project(debugVulkanoManySpheres)

file(GLOB_RECURSE VUL_SRC "../../../essentials/src/*.cpp" "../../../optionals/src/*.cpp")
file(GLOB_RECURSE SRC "../src/*.cpp")
add_executable(vulkanoManySpheres "${SRC}" "${VUL_SRC}")
add_executable(debugVulkanoManySpheres "${SRC}" "${VUL_SRC}")
add_dependencies(vulkanoManySpheres shaders)
add_dependencies(debugVulkanoManySpheres shaders)
target_compile_options(vulkanoManySpheres PRIVATE "-Wall" "-O3")
target_compile_options(debugVulkanoManySpheres PRIVATE "-Wall" "-g")

#add_compile_definitions(VUL_ENABLE_PROFILER)
add_compile_definitions(VUL_ENABLE_DEBUG_NAMER)

target_link_libraries(vulkanoManySpheres vulkan)
target_link_libraries(debugVulkanoManySpheres vulkan)

target_link_libraries(vulkanoManySpheres glfw)
target_link_libraries(debugVulkanoManySpheres glfw)

target_link_libraries(vulkanoManySpheres ktx)
target_link_libraries(debugVulkanoManySpheres ktx)

target_link_libraries(vulkanoManySpheres OpenEXR-3_2)
target_link_libraries(debugVulkanoManySpheres OpenEXR-3_2)

set(IMGUI_PATH "../../../3rdParty/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp) 
add_library("ImGui" STATIC ${IMGUI_SOURCES})
target_include_directories("ImGui" PUBLIC ${IMGUI_PATH})
target_link_libraries(vulkanoManySpheres ImGui)
target_link_libraries(debugVulkanoManySpheres ImGui)

target_include_directories(vulkanoManySpheres PUBLIC "../include" "../../../3rdParty/" "../../../essentials/include/" "../../../optionals/include/")
target_include_directories(debugVulkanoManySpheres PUBLIC "../include" "../../../3rdParty/"  "../../../essentials/include/" "../../../optionals/include/")
