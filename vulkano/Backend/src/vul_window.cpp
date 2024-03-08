#include"../Headers/vul_window.hpp"
#include <GLFW/glfw3.h>
#include<stdexcept>

namespace vulB{

VulWindow::VulWindow(int width, int height, std::string name) : width{width}, height{height}, windowName{name}
{
    InitWindow();
}

VulWindow::~VulWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulWindow::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
}

void VulWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS){
        throw std::runtime_error("Failed to create window surface in vul_window file");
    }
}

void VulWindow::frameBufferResizeCallback(GLFWwindow *window, int windowWidth, int windowHeight)
{
    auto vulwindow = reinterpret_cast<VulWindow *>(glfwGetWindowUserPointer(window));
    vulwindow->frameBufferResized = true;
    vulwindow->width = windowWidth;
    vulwindow->height = windowHeight;
}

}
