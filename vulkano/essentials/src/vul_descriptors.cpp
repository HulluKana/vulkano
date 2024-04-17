#include "vul_debug_tools.hpp"
#include<vul_descriptors.hpp>
 
// std
#include <cassert>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan_core.h>
 
namespace vulB {
 
// *************** Descriptor Set Layout Builder *********************

VulDescriptorSetLayout::Builder &VulDescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count) {
    assert(bindings.count(binding) == 0 && "Binding already in use");
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;
    bindings[binding] = layoutBinding;
    return *this;
}

std::unique_ptr<VulDescriptorSetLayout> VulDescriptorSetLayout::Builder::build() const {
    return std::make_unique<VulDescriptorSetLayout>(vulDevice, bindings);
}

// *************** Descriptor Set Layout *********************

VulDescriptorSetLayout::VulDescriptorSetLayout(
        VulDevice &vulDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
    : vulDevice{vulDevice}, bindings{bindings} {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv : bindings) {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(
                    vulDevice.device(),
                    &descriptorSetLayoutInfo,
                    nullptr,
                    &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        VUL_NAME_VK(descriptorSetLayout)
    }

VulDescriptorSetLayout::~VulDescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(vulDevice.device(), descriptorSetLayout, nullptr);
}

// *************** Descriptor Pool Builder *********************

VulDescriptorPool::Builder &VulDescriptorPool::Builder::addPoolSize(
        VkDescriptorType descriptorType, uint32_t count) {
    poolSizes.push_back({descriptorType, count});
    return *this;
}

VulDescriptorPool::Builder &VulDescriptorPool::Builder::setPoolFlags(
        VkDescriptorPoolCreateFlags flags) {
    poolFlags = flags;
    return *this;
}
VulDescriptorPool::Builder &VulDescriptorPool::Builder::setMaxSets(uint32_t count) {
    maxSets = count;
    return *this;
}

std::unique_ptr<VulDescriptorPool> VulDescriptorPool::Builder::build() const {
    return std::make_unique<VulDescriptorPool>(vulDevice, maxSets, poolFlags, poolSizes);
}

// *************** Descriptor Pool *********************

VulDescriptorPool::VulDescriptorPool(
        VulDevice &vulDevice,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize> &poolSizes)
    : vulDevice{vulDevice} {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(vulDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
                VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

        VUL_NAME_VK(descriptorPool)
    }

VulDescriptorPool::~VulDescriptorPool() {
    vkDestroyDescriptorPool(vulDevice.device(), descriptorPool, nullptr);
}

bool VulDescriptorPool::allocateDescriptorSet(
        const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;

    if (vkAllocateDescriptorSets(vulDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
        return false;
    }

    VUL_NAME_VK(descriptor)
    return true;
}

void VulDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const {
    vkFreeDescriptorSets(
            vulDevice.device(),
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());
}

void VulDescriptorPool::resetPool() {
    vkResetDescriptorPool(vulDevice.device(), descriptorPool, 0);
}

// *************** Descriptor Writer *********************

VulDescriptorSet::VulDescriptorSet(std::shared_ptr<VulDescriptorSetLayout> setLayout, VulDescriptorPool &pool)
    : m_setLayout{setLayout}, m_pool{pool} {}

void VulDescriptorSet::free()
{
    std::vector<VkDescriptorSet> sets = {m_set};
    m_pool.freeDescriptors(sets);
    m_hasSet = false;
}

VulDescriptorSet &VulDescriptorSet::writeBuffer(
        uint32_t binding, VkDescriptorBufferInfo *bufferInfo, uint32_t descriptorCount) {
    assert(m_setLayout->bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = m_setLayout->bindings[binding];
    assert(bindingDescription.descriptorCount == descriptorCount && "Binding some amount of descriptor infos, but binding expects different amount");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = bindingDescription.descriptorType;
    write.dstBinding = binding;
    write.pBufferInfo = bufferInfo;
    write.descriptorCount = descriptorCount;

    m_writes.push_back(write);
    
    DescriptorInfo descInfo{};
    for (uint32_t i = 0; i < descriptorCount; i++)
        descInfo.bufferInfos.push_back(bufferInfo[i]);
    descriptorInfos.push_back(descInfo);
    return *this;
}

VulDescriptorSet &VulDescriptorSet::writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo, uint32_t descriptorCount) {
    assert(m_setLayout->bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = m_setLayout->bindings[binding];
    assert(bindingDescription.descriptorCount == descriptorCount && "Binding some amount of descriptor infos, but binding expects different amount");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = bindingDescription.descriptorType;
    write.dstBinding = binding;
    write.pImageInfo = imageInfo;
    write.descriptorCount = descriptorCount;

    m_writes.push_back(write);

    DescriptorInfo descInfo{};
    for (uint32_t i = 0; i < descriptorCount; i++)
        descInfo.imageInfos.push_back(imageInfo[i]);
    descriptorInfos.push_back(descInfo);
    return *this;
}

bool VulDescriptorSet::build() {
    bool success = m_pool.allocateDescriptorSet(m_setLayout->getDescriptorSetLayout(), m_set);
    if (!success) {
        return false;
    }
    overwrite();
    m_hasSet = true;
    return true;
}

void VulDescriptorSet::update()
{
    VUL_PROFILE_FUNC()

    for (size_t i = 0; i < m_writes.size(); i++){
        if (descriptorInfos[i].bufferInfos.size() > 0){
            m_writes[i].pBufferInfo = descriptorInfos[i].bufferInfos.data();
            m_writes[i].descriptorCount = descriptorInfos[i].bufferInfos.size();
            if (m_writes[i].descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
                m_writes[i].descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                fprintf(stderr, "Invalid descriptor type while updating descriptorSet");
            continue;
        }
        if (descriptorInfos[i].imageInfos.size() > 0){
            m_writes[i].pImageInfo = descriptorInfos[i].imageInfos.data();
            m_writes[i].descriptorCount = descriptorInfos[i].imageInfos.size();
            if (m_writes[i].descriptorType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE &&
                m_writes[i].descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE &&
                m_writes[i].descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                fprintf(stderr, "Invalid descriptor type while updating descriptorSet");
            continue;
        }
    }
    overwrite();
    m_hasSet = true;
}

void VulDescriptorSet::overwrite() {
    VUL_PROFILE_FUNC()

    for (auto &write : m_writes) {
        write.dstSet = m_set;
    }
    vkUpdateDescriptorSets(m_pool.vulDevice.device(), m_writes.size(), m_writes.data(), 0, nullptr);
}
 
}
