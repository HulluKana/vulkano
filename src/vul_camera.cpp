#include <glm/gtc/constants.hpp>
#include <vul_debug_tools.hpp>
#include<vul_camera.hpp>

#include<cassert>
#include<limits>

namespace vul{

void VulCamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) 
{
    projectionMatrix = glm::mat4{1.0f};
    projectionMatrix[0][0] = 2.f / (right - left);
    projectionMatrix[1][1] = 2.f / (bottom - top);
    projectionMatrix[2][2] = 1.f / (far - near);
    projectionMatrix[3][0] = -(right + left) / (right - left);
    projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    projectionMatrix[3][2] = -near / (far - near);
}
    
void VulCamera::setPerspectiveProjection(float fovY, float aspect, float near, float far) 
{
    VUL_PROFILE_FUNC()
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f && "I think if this fails the given aspect is too small");
    const float tanHalffovY = -tan(fovY / 2.f);
    projectionMatrix = glm::mat4{0.0f};
    projectionMatrix[0][0] = 1.f / (aspect * tanHalffovY);
    projectionMatrix[1][1] = 1.f / (tanHalffovY);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);
}

void VulCamera::setViewDirection(glm::vec3 direction, glm::vec3 up) {
    assert(direction != glm::vec3(0.0f, 0.0f, 0.0f) && "Direction vector has to have a length above 0");
    assert(up != glm::vec3(0.0f, 0.0f, 0.0f) && "Up vector has to have a length above 0");
    const glm::vec3 w{glm::normalize(direction)};
    const glm::vec3 u{glm::normalize(glm::cross(w, up))};
    const glm::vec3 v{glm::cross(w, u)};

    viewMatrix = glm::mat4{1.f};
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    viewMatrix[3][0] = -glm::dot(u, pos);
    viewMatrix[3][1] = -glm::dot(v, pos);
    viewMatrix[3][2] = -glm::dot(w, pos);
}

void VulCamera::setViewTarget(glm::vec3 target, glm::vec3 up) {
    assert(pos != target && "The view target pos cant be the same as camera pos");
    setViewDirection(target - pos, up);
}

void VulCamera::updateXYZ() {
    VUL_PROFILE_FUNC()
    const float c3 = glm::cos(rot.z);
    const float s3 = glm::sin(rot.z);
    const float c2 = glm::cos(rot.y);
    const float s2 = glm::sin(rot.y);
    const float c1 = glm::cos(rot.x);
    const float s1 = glm::sin(rot.x);
    const glm::vec3 u{(c2 * c3), (-c2 * s3), (s2)};
    const glm::vec3 v{(c1 * s3 + c3 * s1 * s2), (c1 * c3 - s1 * s2 * s3), (-c2 * s1)};
    const glm::vec3 w{(s1 * s3 - c1 * c3 * s2), (c3 * s1 + c1 * s2 * s3), (c1 * c2)};
    viewMatrix = glm::mat4{1.f};
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    viewMatrix[3][0] = -glm::dot(u, pos);
    viewMatrix[3][1] = -glm::dot(v, pos);
    viewMatrix[3][2] = -glm::dot(w, pos);
}

void VulCamera::applyInputs(GLFWwindow *window, float dt, uint32_t screenHeight)
{
    VUL_PROFILE_FUNC()
    if (glfwGetKey(window, keys.toggleGUI) == GLFW_PRESS) hideGUIpressed = true;
    if (glfwGetKey(window, keys.toggleGUI) == GLFW_RELEASE && hideGUIpressed){
        hideGui = !hideGui;
        hideGUIpressed = false;
    }
    if (hideGui) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    moveSpeed = baseMoveSpeed;
    if (glfwGetKey(window, keys.moveFaster) == GLFW_PRESS) moveSpeed = baseMoveSpeed * speedChanger;
    if (glfwGetKey(window, keys.moveSlower) == GLFW_PRESS) moveSpeed = baseMoveSpeed / speedChanger;

    if (glfwGetKey(window, keys.resetAll) == GLFW_PRESS){
        pos = glm::vec3(0.0f, 0.0f, 0.0f);
        rot = glm::vec3(0.0f, 0.0f, 0.0f);
    }


    glm::vec3 keyRotate = glm::vec3(0.0f);
    glm::vec3 mouseRotate = glm::vec3(0.0f);
    if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) keyRotate.y += 1.0f; 
    if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) keyRotate.y -= 1.0f; 
    if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) keyRotate.x += 1.0f; 
    if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) keyRotate.x -= 1.0f; 
    if (glfwGetKey(window, keys.rollRight) == GLFW_PRESS) keyRotate.z += 1.0f; 
    if (glfwGetKey(window, keys.rollLeft) == GLFW_PRESS) keyRotate.z -= 1.0f; 

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS || hideGui){
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
        rot += glm::normalize(keyRotate) * lookSpeed * dt;
    if (glm::dot(mouseRotate, mouseRotate) > std::numeric_limits<float>::epsilon())
        rot += glm::normalize(mouseRotate) * sensitivity * dt;

    rot.x = glm::mod(rot.x, glm::two_pi<float>());
    rot.y = glm::mod(rot.y, glm::two_pi<float>());
    rot.z = glm::mod(rot.z, glm::two_pi<float>());


    float yaw = -rot.y;
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
        pos += glm::normalize(moveDir) * moveSpeed * dt;
}

}
