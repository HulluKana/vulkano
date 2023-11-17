#include"../Headers/vul_movement_controller.hpp"

#include<iostream>

using namespace vul;
namespace vulB{

MovementController::MovementController()
{
}

void MovementController::modifyValues(GLFWwindow *window, Vul3DObject &object)
{
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
        object.transform.posOffset = glm::vec3(0.0f, 0.0f, 0.0f);
        object.transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

void MovementController::rotate(GLFWwindow *window, float dt, Vul3DObject &object, int screenWidth, int screenHeight)
{
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

        mouseRotate.x -= (float)(mouseY - (screenHeight / 2));
        mouseRotate.y += (float)(mouseX - (screenWidth / 2));

        glfwSetCursorPos(window, screenWidth / 2, screenHeight / 2);
    }

    if (glm::dot(keyRotate, keyRotate) > std::numeric_limits<float>::epsilon())
        object.transform.rotation += glm::normalize(keyRotate) * lookSpeed * dt;
    if (glm::dot(mouseRotate, mouseRotate) > std::numeric_limits<float>::epsilon())
        object.transform.rotation += glm::normalize(mouseRotate) * sensitivity * dt;

    
    object.transform.rotation.x = glm::mod(object.transform.rotation.x, glm::two_pi<float>());
    object.transform.rotation.y = glm::mod(object.transform.rotation.y, glm::two_pi<float>());
}

void MovementController::move(GLFWwindow *window, float dt, Vul3DObject &object)
{
    float yaw = object.transform.rotation.y;
    const glm::vec3 forwardDir = glm::vec3(sin(yaw), 0.0f, cos(yaw));
    const glm::vec3 rightDir = glm::vec3(forwardDir.z, 0.0f, -forwardDir.x);
    const glm::vec3 upDir = glm::vec3(0.0f, -1.0f, 0.0f);

    glm::vec3 moveDir = glm::vec3(0.0f);
    if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir; 
    if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir; 
    if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
    if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir; 
    if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
    if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
        object.transform.posOffset += glm::normalize(moveDir) * moveSpeed * dt;
}

}