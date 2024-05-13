cmake_minimum_required(VERSION "3.22.1")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

add_custom_target(shaders)

file(GLOB_RECURSE SHADERS_SRC "../Shaders/*.vert" "../Shaders/*.frag" "../Shaders/*.rgen" "../Shaders/*.rahit" "../Shaders/*.rchit" "../Shaders/*.rmiss")
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



project(vulkanoRaytracer)
project(debugVulkanoRaytracer)

file(GLOB_RECURSE VUL_SRC "../../../essentials/src/*.cpp" "../../../optionals/src/*.cpp")
file(GLOB_RECURSE SRC "../src/*.cpp")
add_executable(vulkanoRaytracer "${SRC}" "${VUL_SRC}")
add_executable(debugVulkanoRaytracer "${SRC}" "${VUL_SRC}")
add_dependencies(vulkanoRaytracer shaders)
add_dependencies(debugVulkanoRaytracer shaders)
target_compile_options(vulkanoRaytracer PRIVATE "-Wall" "-O3")
target_compile_options(debugVulkanoRaytracer PRIVATE "-Wall" "-g")

add_compile_definitions(VUL_ENABLE_PROFILER)
add_compile_definitions(VUL_ENABLE_DEBUG_NAMER)

target_link_libraries(vulkanoRaytracer vulkan)
target_link_libraries(debugVulkanoRaytracer vulkan)

target_link_libraries(vulkanoRaytracer glfw)
target_link_libraries(debugVulkanoRaytracer glfw)

target_link_libraries(vulkanoRaytracer ktx)
target_link_libraries(debugVulkanoRaytracer ktx)

target_link_libraries(vulkanoRaytracer OpenEXR-3_2)
target_link_libraries(debugVulkanoRaytracer OpenEXR-3_2)

set(IMGUI_PATH "../../../3rdParty/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp) 
add_library("ImGui" STATIC ${IMGUI_SOURCES})
target_include_directories("ImGui" PUBLIC ${IMGUI_PATH})
target_link_libraries(vulkanoRaytracer ImGui)
target_link_libraries(debugVulkanoRaytracer ImGui)

target_include_directories(vulkanoRaytracer PUBLIC "../include" "../../../3rdParty/" "../../../essentials/include/" "../../../optionals/include/" "/usr/include/Imath")
target_include_directories(debugVulkanoRaytracer PUBLIC "../include" "../../../3rdParty/"  "../../../essentials/include/" "../../../optionals/include/" "/usr/include/Imath")
