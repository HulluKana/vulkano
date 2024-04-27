#include <vul_debug_tools.hpp>
#include<vul_movement_controller.hpp>

using namespace vul;
namespace vulB{

MovementController::MovementController()
{
}

void MovementController::modifyValues(GLFWwindow *window, transform3D &transform)
{
    VUL_PROFILE_FUNC()
    if (glfwGetKey(window, keys.toggleGUI) == GLFW_PRESS) hideGUIpressed = true;
    if (glfwGetKey(window, keys.toggleGUI) == GLFW_RELEASE && hideGUIpressed){
        hideGUI = !hideGUI;
        hideGUIpressed = false;
    }
    if (hideGUI) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    moveSpeed = baseMoveSpeed;
    if (glfwGetKey(window, keys.moveFaster) == GLFW_PRESS) moveSpeed = baseMoveSpeed * speedChanger;
    if (glfwGetKey(window, keys.moveSlower) == GLFW_PRESS) moveSpeed = baseMoveSpeed / speedChanger;

    if (glfwGetKey(window, keys.resetAll) == GLFW_PRESS){
        transform.pos = glm::vec3(0.0f, 0.0f, 0.0f);
        transform.rot = glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

void MovementController::rotate(GLFWwindow *window, float dt, transform3D &transform, int screenWidth, int screenHeight)
{
    VUL_PROFILE_FUNC()
    glm::vec3 keyRotate = glm::vec3(0.0f);
    glm::vec3 mouseRotate = glm::vec3(0.0f);
    if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) keyRotate.y += 1.0f; 
    if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) keyRotate.y -= 1.0f; 
    if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) keyRotate.x += 1.0f; 
    if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) keyRotate.x -= 1.0f; 
    if (glfwGetKey(window, keys.rollRight) == GLFW_PRESS) keyRotate.z += 1.0f; 
    if (glfwGetKey(window, keys.rollLeft) == GLFW_PRESS) keyRotate.z -= 1.0f; 


    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS || hideGUI){
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        static double lastMouseX = mouseX;
        static double lastMouseY = mouseY;

        mouseRotate.x -= (float)(mouseY - lastMouseY);
        mouseRotate.y += (float)(mouseX - lastMouseX);

        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    if (glm::dot(keyRotate, keyRotate) > std::numeric_limits<float>::epsilon())
        transform.rot += glm::normalize(keyRotate) * lookSpeed * dt;
    if (glm::dot(mouseRotate, mouseRotate) > std::numeric_limits<float>::epsilon())
        transform.rot += glm::normalize(mouseRotate) * sensitivity * dt;

    
    transform.rot.x = glm::mod(transform.rot.x, glm::two_pi<float>());
    transform.rot.y = glm::mod(transform.rot.y, glm::two_pi<float>());
}

void MovementController::move(GLFWwindow *window, float dt, transform3D &transform)
{
    VUL_PROFILE_FUNC()
    float yaw = -transform.rot.y;
    const glm::vec3 forwardDir = glm::vec3(sin(yaw), 0.0f, cos(yaw));
    const glm::vec3 rightDir = glm::vec3(-forwardDir.z, 0.0f, forwardDir.x);
    const glm::vec3 upDir = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 moveDir = glm::vec3(0.0f);
    if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir; 
    if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir; 
    if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
    if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir; 
    if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
    if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
        transform.pos += glm::normalize(moveDir) * moveSpeed * dt;
}

}
