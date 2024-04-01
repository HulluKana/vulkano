#include<vul_camera.hpp>

#include<cassert>
#include<limits>

namespace vulB{

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
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f && "I think if this fails the given aspect is too small");
    const float tanHalffovY = -tan(fovY / 2.f);
    projectionMatrix = glm::mat4{0.0f};
    projectionMatrix[0][0] = 1.f / (aspect * tanHalffovY);
    projectionMatrix[1][1] = 1.f / (tanHalffovY);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);
}

void VulCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
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
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);
}

void VulCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    assert(position != target && "The view target position cant be the same as camera position");
    setViewDirection(position, target - position, up);
}

void VulCamera::setViewXYZ(glm::vec3 position, glm::vec3 rotation) {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.y);
    const float s2 = glm::sin(rotation.y);
    const float c1 = glm::cos(rotation.x);
    const float s1 = glm::sin(rotation.x);
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
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);
}

}
