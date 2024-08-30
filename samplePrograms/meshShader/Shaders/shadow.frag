#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_nonuniform_qualifier : enable


#include"../include/host_device.hpp"
#include"../../../include/vul_scene.hpp"

layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec2 fragTexCoord;
layout (location = 2) in flat uint matIdx;

layout(set = 0, binding = 7) writeonly uniform imageCube shadowMap;
layout(set = 0, binding = 8) uniform sampler2D textures[];
layout(set = 0, binding = 9) readonly buffer MaterialBuffer{PackedMaterial materials[];};

layout(push_constant) uniform Push{uint layerIdx;};

layout (early_fragment_tests) in;

void main()
{
    imageStore(shadowMap, ivec3(ivec2(gl_FragCoord.xy), layerIdx), texture(textures[materials[matIdx].colorTextureIndex], fragTexCoord));
}
