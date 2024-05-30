#pragma once

#include <vulkano_program.hpp>

struct RasResources {
    std::shared_ptr<vulB::VulBuffer> objDataBuf;
    std::array<std::unique_ptr<vulB::VulBuffer>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> ubos;
    std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> descSets;
};
RasResources createRasterizationResources(vul::Vulkano &vulkano);
void updateRasUbo(const vul::Vulkano &vulkano, RasResources &res, bool fullUpdate);
