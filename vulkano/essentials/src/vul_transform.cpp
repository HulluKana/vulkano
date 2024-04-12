#include <vul_debug_tools.hpp>
#include<vul_transform.hpp>

namespace vul{

glm::mat4 transform3D::transformMat()
{
    VUL_PROFILE_FUNC()
    // Rotations correspond to Tait-bryan angles of X(1), Y(2), Z(3)
    // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
    /* Following function is basically
        auto transform = glm::translate(glm::mat4{1.0f, possOffset});
        transform = glm::rotate(transform, rot.x, {1.0f, 0.0f, 0.0f});
        transform = glm::rotate(transform, rot.y, {0.0f, 1.0f, 0.0f});
        transform = glm::rotate(transform, rot.z, {0.0f, 0.0f, 1.0f});
        transform = glm::scale(transform, scale);
    but faster. The code above rotates around an arbitrary axis where as the code
    below rotates around x, y and z axis, which is much faster to do.
    The math is based on the Tait-Bryan angles second equation, which you can find from the wikipedia link */
    const float c3 = glm::cos(rot.z);
    const float s3 = glm::sin(rot.z);
    const float c2 = glm::cos(rot.y);
    const float s2 = glm::sin(rot.y);
    const float c1 = glm::cos(rot.x);
    const float s1 = glm::sin(rot.x);

    return glm::mat4{
        {
            scale.x * (c2 * c3),
            scale.x * (-c2 * s3),
            scale.x * s2,
            0.0f,
        },
        {
            scale.y * (c1 * s3 + c3 * s1 * s2),
            scale.y * (c1 * c3 - s1 * s2 * s3),
            scale.y * (-c2 * s1),
            0.0f,
        },
        {
            scale.z * (s1 * s3 - c1 * c3 * s2),
            scale.z * (c3 * s1 + c1 * s2 * s3),
            scale.z * (c1 * c2),
            0.0f,
        },
        {pos.x, pos.y, pos.z, 1.0f}
    };
}

};
