#include <resources.hpp>

ReservoirGrid createReservoirGrid(const vul::Scene &scene, const vulB::VulDevice &device)
{
    const int minX = std::floor(scene.minPos.x);
    const int minY = std::floor(scene.minPos.y);
    const int minZ = std::floor(scene.minPos.z);

    const uint32_t width = std::ceil(scene.maxPos.x - scene.minPos.x);
    const uint32_t height = std::ceil(scene.maxPos.y - scene.minPos.y);
    const uint32_t depth = std::ceil(scene.maxPos.z - scene.minPos.z);
    const uint32_t reservoirCount = width * height * depth * RESERVOIRS_PER_CELL;

    glm::vec<4, int> minPos = {minX, minY, minZ, -69};
    glm::vec<4, uint32_t> dims = {width, height, depth, 420};
    Reservoir defaultReservoir{0, 0.0f, 0.0f};
    uint8_t *data = static_cast<uint8_t *>(malloc(sizeof(minPos) + sizeof(dims) + sizeof(Reservoir) * reservoirCount));
    memcpy(data, &minPos, sizeof(minPos));
    memcpy(data + sizeof(minPos), &dims, sizeof(dims));
    for (uint32_t i = 0; i < reservoirCount; i++) memcpy(data + sizeof(minPos) + sizeof(dims) + i * sizeof(Reservoir), &defaultReservoir, sizeof(Reservoir));

    ReservoirGrid reservoirGrid{};
    reservoirGrid.width = width;
    reservoirGrid.height = height;
    reservoirGrid.depth = depth;
    reservoirGrid.minX = minX;
    reservoirGrid.minY = minY;
    reservoirGrid.minZ = minZ;
    reservoirGrid.buffer = std::make_unique<vulB::VulBuffer>(device);
    reservoirGrid.buffer->loadData(data, sizeof(minPos) + sizeof(dims) + sizeof(Reservoir) * reservoirCount, 1);
    assert(reservoirGrid.buffer->createBuffer(true, static_cast<vulB::VulBuffer::Usage>(vulB::VulBuffer::usage_ssbo | vulB::VulBuffer::usage_transferDst)) == VK_SUCCESS);

    free(data);
    return reservoirGrid;
}

std::unique_ptr<vulB::VulDescriptorSet> createRtDescSet(const vul::Vulkano &vulkano, const vul::VulAs &as, const std::unique_ptr<vul::VulImage> &rtImg, const std::unique_ptr<vulB::VulBuffer> &ubo, const std::unique_ptr<vul::VulImage> &enviromentMap, const std::vector<std::unique_ptr<vulB::VulBuffer>> &reservoirsBuffers, const std::unique_ptr<vul::VulImage> &hitCacheBuffer, const std::unique_ptr<vulB::VulBuffer> &cellsBuffer)
{
    std::vector<vul::Vulkano::Descriptor> descriptors;
    vul::Vulkano::Descriptor desc{};
    desc.count = 1;

    desc.type = vul::Vulkano::DescriptorType::storageImage;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::frag};
    desc.content = rtImg.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::ubo;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rmiss, vul::Vulkano::ShaderStage::rchit};
    desc.content = ubo.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::ssbo;
    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::rahit};
    desc.content = vulkano.scene.indexBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.vertexBuffer.get();
    descriptors.push_back(desc);

    desc.stages = {vul::Vulkano::ShaderStage::rchit};
    desc.content = vulkano.scene.indexBuffer.get();
    desc.content = vulkano.scene.normalBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.tangentBuffer.get();
    descriptors.push_back(desc);

    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::rahit};
    desc.content = vulkano.scene.indexBuffer.get();
    desc.content = vulkano.scene.uvBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.materialBuffer.get();
    descriptors.push_back(desc);
    desc.content = vulkano.scene.primInfoBuffer.get();
    descriptors.push_back(desc);

    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::comp};
    desc.content = vulkano.scene.lightsBuffer.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::upSsbo;
    desc.content = reservoirsBuffers.data();
    desc.count = reservoirsBuffers.size();
    descriptors.push_back(desc);

    desc.type = {vul::Vulkano::DescriptorType::ssbo};
    desc.content = cellsBuffer.get();
    desc.count = 1;
    descriptors.push_back(desc);

    desc.type = {vul::Vulkano::DescriptorType::storageImage};
    desc.content = hitCacheBuffer.get();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::spCombinedImgSampler;
    desc.stages = {vul::Vulkano::ShaderStage::rchit, vul::Vulkano::ShaderStage::rahit};
    desc.content = vulkano.scene.images.data();
    desc.count = vulkano.scene.images.size();
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::combinedImgSampler;
    desc.stages = {vul::Vulkano::ShaderStage::rmiss};
    desc.content = enviromentMap.get();
    desc.count = 1;
    descriptors.push_back(desc);

    desc.type = vul::Vulkano::DescriptorType::accelerationStructure;
    desc.stages = {vul::Vulkano::ShaderStage::rgen, vul::Vulkano::ShaderStage::rchit};
    desc.count = 1;
    desc.content = &as;
    descriptors.push_back(desc);

    return vulkano.createDescriptorSet(descriptors);
}

vul::Vulkano::RenderData createRenderData(const vul::Vulkano &vulkano, const vulB::VulDevice &device, const std::array<std::shared_ptr<vulB::VulDescriptorSet>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &descSets)
{
    vulB::VulPipeline::PipelineConfigInfo pipConf{};
    pipConf.attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, {1, 1, VK_FORMAT_R32G32_SFLOAT, 0}};
    pipConf.bindingDescriptions = {{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}, {1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}};
    pipConf.colorAttachmentFormats = {vulkano.vulRenderer.getSwapChainColorFormat()};
    pipConf.depthAttachmentFormat = vulkano.vulRenderer.getDepthFormat();
    pipConf.setLayouts = {descSets[0]->getLayout()->getDescriptorSetLayout()};
    pipConf.enableColorBlending = false;
    pipConf.cullMode = VK_CULL_MODE_NONE;

    vul::Vulkano::RenderData renderData{};
    renderData.is3d = false;
    renderData.depthImageMode = vulB::VulRenderer::DepthImageMode::noDepthImage;
    renderData.swapChainImageMode = vulB::VulRenderer::SwapChainImageMode::clearPreviousStoreCurrent;
    renderData.sampleFromDepth = false;
    for (int i = 0; i < vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT; i++) renderData.descriptorSets[i].push_back(descSets[i]);
    renderData.pipeline = std::make_shared<vulB::VulPipeline>(device, "../bin/raytrace.vert.spv", "../bin/raytrace.frag.spv", pipConf);

    return renderData;
}

void resizeRtImgs(vul::Vulkano &vulkano, std::array<std::unique_ptr<vul::VulImage>, vulB::VulSwapChain::MAX_FRAMES_IN_FLIGHT> &rtImgs)
{
    VkCommandBuffer cmdBuf = vulkano.getVulDevice().beginSingleTimeCommands();
    for (size_t i = 0; i < rtImgs.size(); i++) {
        rtImgs[i] = std::make_unique<vul::VulImage>(vulkano.getVulDevice());
        rtImgs[i]->keepRegularRaw2d32bitRgbaEmpty(vulkano.getSwapChainExtent().width, vulkano.getSwapChainExtent().height);
        rtImgs[i]->createDefaultImage(vul::VulImage::ImageType::storage2d, cmdBuf);
    }
    vulkano.getVulDevice().endSingleTimeCommands(cmdBuf);

    for (size_t i = 0; i < rtImgs.size(); i++) {
        vulkano.renderDatas[0].descriptorSets[i][0]->descriptorInfos[0].imageInfos[0].imageView = rtImgs[i]->getImageView();
        vulkano.renderDatas[0].descriptorSets[i][0]->update();
    }
}

void updateUbo(const vul::Vulkano &vulkano, std::unique_ptr<vulB::VulBuffer> &ubo)
{
    GlobalUbo uboData{};
    uboData.inverseViewMatrix = glm::inverse(vulkano.camera.getView());
    uboData.inverseProjectionMatrix = glm::inverse(vulkano.camera.getProjection());
    uboData.cameraPosition = glm::vec4(vulkano.cameraTransform.pos, 0.0f);

    uboData.ambientLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);

    // Pixel spread angle is from equation 30 from
    // https://media.contentapi.ea.com/content/dam/ea/seed/presentations/2019-ray-tracing-gems-chapter-20-akenine-moller-et-al.pdf
    uboData.pixelSpreadAngle = atan((2.0f * tan(vul::settings::cameraProperties.fovY / 2.0f)) / static_cast<float>(vulkano.getSwapChainExtent().height));
    uboData.padding1 = 69;

    ubo->writeData(&uboData, sizeof(GlobalUbo), 0);
}
