cmake_minimum_required(VERSION "3.22.1")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

add_custom_target(shaders)

file(GLOB_RECURSE SHADERS_SRC "../Shaders/*.vert" "../Shaders/*.frag" "../Shaders/*.comp")
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



project(vulkanoRasteriser)
project(debugVulkanoRasteriser)

file(GLOB_RECURSE VUL_SRC "../../../src/*.cpp")
file(GLOB_RECURSE SRC "../src/*.cpp")
add_executable(vulkanoRasteriser "${SRC}" "${VUL_SRC}")
add_executable(debugVulkanoRasteriser "${SRC}" "${VUL_SRC}")
add_dependencies(vulkanoRasteriser shaders)
add_dependencies(debugVulkanoRasteriser shaders)
target_compile_options(vulkanoRasteriser PRIVATE "-Wall" "-O3" "-std=c++20")
target_compile_options(debugVulkanoRasteriser PRIVATE "-Wall" "-g" "-std=c++20")

#add_compile_definitions(VUL_ENABLE_PROFILER)
add_compile_definitions(VUL_ENABLE_DEBUG_NAMER)

target_link_libraries(vulkanoRasteriser vulkan)
target_link_libraries(debugVulkanoRasteriser vulkan)

target_link_libraries(vulkanoRasteriser glfw)
target_link_libraries(debugVulkanoRasteriser glfw)

target_link_libraries(vulkanoRasteriser ktx)
target_link_libraries(debugVulkanoRasteriser ktx)

target_link_libraries(vulkanoRasteriser OpenEXR-3_2)
target_link_libraries(debugVulkanoRasteriser OpenEXR-3_2)

set(IMGUI_PATH "../../../3rdParty/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp) 
add_library("ImGui" STATIC ${IMGUI_SOURCES})
target_include_directories("ImGui" PUBLIC ${IMGUI_PATH})
target_link_libraries(vulkanoRasteriser ImGui)
target_link_libraries(debugVulkanoRasteriser ImGui)

set(MESH_OPTIMIZER_PATH "../../../3rdParty/meshoptimizer")
file(GLOB MESH_OPTIMIZER_SOURCES ${MESH_OPTIMIZER_PATH}/src/*.cpp) 
add_library("MeshOptimizer" STATIC ${MESH_OPTIMIZER_SOURCES})
target_compile_options(MeshOptimizer PRIVATE "-O3")
target_link_libraries(vulkanoRasteriser MeshOptimizer)
target_link_libraries(debugVulkanoRasteriser MeshOptimizer)

target_include_directories(vulkanoRasteriser PUBLIC "../include" "../../../3rdParty/" "../../../include/")
target_include_directories(debugVulkanoRasteriser PUBLIC "../include" "../../../3rdParty/"  "../../../include/")
