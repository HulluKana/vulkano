#include"../../vulkano_GUI_tools.hpp"
#include <string>

namespace vul{

namespace GUI{

bool DragFloat3(const char *label, glm::vec3 &v, float v_max, float v_min, float v_speed, uint32_t decimals, ImGuiSliderFlags flags)
{
    float v_array[3] = {v.x, v.y, v.z};
    std::string format("%.");
    format += std::to_string(decimals) + "f";

    bool returnValue = ImGui::DragFloat3(label, v_array, v_speed, v_min, v_max, format.c_str(), flags);
    
    v = glm::vec3(v_array[0], v_array[1], v_array[2]);

    return returnValue;
}

}

}
