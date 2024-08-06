#pragma once

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>

#include<string>

namespace vul {

class VulWindow{
    public:
        VulWindow(int width, int height, std::string name);
        ~VulWindow();
        
        /* These 2 lines remove the copy constructor and operator from VulWindow class.
        Because I'm using a pointer to GLFWwindow and stuff is initialized by constructor and removed by destructor,
        copying the pointer and then removing the original pointer leaves me with pointer pointing to nothing, possibly leading to VERY nasty bugs */
        VulWindow(const VulWindow &) = delete;
        VulWindow &operator=(const VulWindow &) = delete;

        bool shouldClose() {return glfwWindowShouldClose(window);}
        VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
        bool wasWindowResized() { return frameBufferResized; }
        void resetWindowResizedFlag() { frameBufferResized = false; }
        GLFWwindow *getGLFWwindow() const {return window;}

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    private:
        static void frameBufferResizeCallback(GLFWwindow *window, int windowWidth, int windowHeight);
        void InitWindow();

        int width;
        int height;
        bool frameBufferResized = false;

        std::string windowName;
        GLFWwindow *window;
};
}