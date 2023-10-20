cmake_minimum_required(VERSION "3.22.1")

project(vulkano)
project(debugVulkano)

file(GLOB_RECURSE SRC "../vulkano/Backend/src/*.cpp")
add_executable(vulkano "../main.cpp" "${SRC}")
add_executable(debugVulkano "../main.cpp" "${SRC}")
target_compile_options(vulkano PRIVATE "-Wall")
target_compile_options(debugVulkano PRIVATE "-Wall" "-g")

find_package(Vulkan REQUIRED)
target_link_libraries(vulkano ${Vulkan_LIBRARIES})
target_link_libraries(debugVulkano ${Vulkan_LIBRARIES})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("Dependencies/GLFW")
target_link_libraries(vulkano glfw)
target_link_libraries(debugVulkano glfw)

set(IMGUI_PATH "Dependencies/imgui")
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp) 
add_library("ImGui" STATIC ${IMGUI_SOURCES})
target_include_directories("ImGui" PUBLIC ${IMGUI_PATH})
target_link_libraries(vulkano ImGui)
target_link_libraries(debugVulkano ImGui)

find_package(glm REQUIRED)
target_link_libraries(vulkano glm::glm)
target_link_libraries(debugVulkano glm::glm)

target_include_directories(vulkano PUBLIC "Dependencies/stb_image/" "Dependencies/tinyObjLoader/" "../vulkano/" "../vulkano/Backend/Headers/")
target_include_directories(debugVulkano PUBLIC "Dependencies/stb_image/" "Dependencies/tinyObjLoader/" "../vulkano/" "../vulkano/Backend/Headers/")