#pragma once

#include"vul_transform.hpp"
#include"vul_window.hpp"

namespace vul{

class MovementController{
    public:
        MovementController();

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

        void modifyValues(GLFWwindow *window, transform3D &transform);
        void rotate(GLFWwindow *window, float dt, transform3D &transform, int screenWidth, int screenHeight);
        void move(GLFWwindow *window, float dt, transform3D &transform);

        bool hideGUI = false;

        KeyMappings keys{};
        float baseMoveSpeed = 3.0f;
        float speedChanger = 2.0f;
        float lookSpeed = 2.0f;
        float sensitivity = 3.0f;

    private:
        float moveSpeed = baseMoveSpeed;
        bool hideGUIpressed = false;
};

}
