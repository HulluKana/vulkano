cmake_minimum_required(VERSION "3.22.1")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

project(vulkanoVideo)
project(debugVulkanoVideo)

file(GLOB_RECURSE VUL_SRC "../../../src/*.cpp")
file(GLOB_RECURSE SRC "../src/*.cpp")
add_executable(vulkanoVideo "${SRC}" "${VUL_SRC}")
add_executable(debugVulkanoVideo "${SRC}" "${VUL_SRC}")
target_compile_options(vulkanoVideo PRIVATE "-Wall" "-std=c++20")
target_compile_options(debugVulkanoVideo PRIVATE "-Wall" "-g" "-std=c++20")

#add_compile_definitions(VUL_ENABLE_PROFILER)
add_compile_definitions(VUL_ENABLE_DEBUG_NAMER)

target_link_libraries(vulkanoVideo vulkan)
target_link_libraries(debugVulkanoVideo vulkan)

target_link_libraries(vulkanoVideo glfw)
target_link_libraries(debugVulkanoVideo glfw)

target_link_libraries(vulkanoVideo ktx)
target_link_libraries(debugVulkanoVideo ktx)

target_link_libraries(vulkanoVideo OpenEXR-3_2)
target_link_libraries(debugVulkanoVideo OpenEXR-3_2)

set(IMGUI_PATH "../../../3rdParty/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp) 
add_library("ImGui" STATIC ${IMGUI_SOURCES})
target_include_directories("ImGui" PUBLIC ${IMGUI_PATH})
target_link_libraries(vulkanoVideo ImGui)
target_link_libraries(debugVulkanoVideo ImGui)

set(MESH_OPTIMIZER_PATH "../../../3rdParty/meshoptimizer")
file(GLOB MESH_OPTIMIZER_SOURCES ${MESH_OPTIMIZER_PATH}/src/*.cpp) 
add_library("MeshOptimizer" STATIC ${MESH_OPTIMIZER_SOURCES})
target_compile_options(MeshOptimizer PRIVATE "-O3")
target_link_libraries(vulkanoVideo MeshOptimizer)
target_link_libraries(debugVulkanoVideo MeshOptimizer)

target_include_directories(vulkanoVideo PUBLIC "../include" "../../../3rdParty/" "../../../include/")
target_include_directories(debugVulkanoVideo PUBLIC "../include" "../../../3rdParty/" "../../../include/")
