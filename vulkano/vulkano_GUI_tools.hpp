#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>
#include"../3rdParty/imgui/imgui.h"

namespace vul{

namespace GUI{

bool DragFloat3(const char *label, glm::vec3 &v, float v_max = 0.0f, float v_min = 0.0f, float v_speed = 1.0f, uint32_t decimals = 3, ImGuiSliderFlags flags = 0);

}

}
