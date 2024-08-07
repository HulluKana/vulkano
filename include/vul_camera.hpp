#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace vul{

class VulCamera{
    public:
        void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
        void setPerspectiveProjection(float fovY, float aspect, float near, float far);

        void setViewDirection(glm::vec3 direction, glm::vec3 up = glm::vec3{0.0f, 1.0f, 0.0f});
        void setViewTarget(glm::vec3 target, glm::vec3 up = glm::vec3{0.0f, 1.0f, 0.0f});
        void updateXYZ();

        const glm::mat4 &getProjection() const {return projectionMatrix;}
        const glm::mat4 &getView() const {return viewMatrix;}

        struct KeyMappings {
            int moveRight = GLFW_KEY_D;
            int moveLeft = GLFW_KEY_A;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_SPACE;
            int moveDown = GLFW_KEY_LEFT_ALT;
            int moveFaster = GLFW_KEY_LEFT_SHIFT;
            int moveSlower = GLFW_KEY_LEFT_CONTROL;
            int rollRight = GLFW_KEY_E;
            int rollLeft = GLFW_KEY_Q;
            int lookRight = GLFW_KEY_RIGHT;
            int lookLeft = GLFW_KEY_LEFT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;
            int toggleGUI = GLFW_KEY_ESCAPE;
            int resetAll = GLFW_KEY_R;
        };

        void applyInputs(GLFWwindow *window, float dt, uint32_t screenHeight);

        bool shouldHideGui() const {return hideGui;}

        glm::vec3 pos;
        glm::vec3 rot;

        KeyMappings keys{};
        float baseMoveSpeed = 3.0f;
        float speedChanger = 2.0f;
        float lookSpeed = 2.0f;
        float sensitivity = 3.0f;

    private:
        float moveSpeed = baseMoveSpeed;
        bool hideGUIpressed = false;
        bool hideGui = false;

    private:
        glm::mat4 projectionMatrix{1.0f};
        glm::mat4 viewMatrix{1.0f};
};

}
