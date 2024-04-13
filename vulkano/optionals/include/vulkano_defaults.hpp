#pragma once

#include<vulkano_program.hpp>

namespace vul
{
namespace defaults
{
    struct Default3dInputData{
        std::shared_ptr<VulImage> multipleBounce2dImage = nullptr;
        std::shared_ptr<VulImage> multipleBounce1dImage = nullptr;
        std::shared_ptr<vulB::VulBuffer> aBuffer = nullptr;
        std::shared_ptr<VulImage> aBufferHeads = nullptr;
        std::shared_ptr<vulB::VulBuffer> aBufferCounter = nullptr;
    };
    Default3dInputData createDefault3dInputData(Vulkano &vulkano);

    struct DefaultRenderDataInputData {
        size_t mainRenderDataIdx;
        size_t oitColoringRenderDataIdx;
        size_t oitCompositingRenderDataIdx;
    };
    DefaultRenderDataInputData createDefaultDescriptors(Vulkano &vulkano, Default3dInputData inputData);
    void createDefault3dRenderSystem(Vulkano &vulkano, DefaultRenderDataInputData inputData);
    void updateDefault3dInputValues(Vulkano &vulkano, DefaultRenderDataInputData inputDataRender, Default3dInputData inputData3d);
}
}
