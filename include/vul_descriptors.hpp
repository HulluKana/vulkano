#pragma once

#include "vul_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace vul {

class VulDescriptorSetLayout {
    public:
        class Builder {
            public:
                Builder(const VulDevice &vulDevice) : vulDevice{vulDevice} {}

                Builder &addBinding(
                        uint32_t binding,
                        VkDescriptorType descriptorType,
                        VkShaderStageFlags stageFlags,
                        const std::vector<VkSampler> &immutableSamplers,
                        uint32_t count);
                std::unique_ptr<VulDescriptorSetLayout> build() const;

            private:
                const VulDevice &vulDevice;
                std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };

        VulDescriptorSetLayout(
                const VulDevice &vulDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~VulDescriptorSetLayout();
        VulDescriptorSetLayout(const VulDescriptorSetLayout &) = delete;
        VulDescriptorSetLayout &operator=(const VulDescriptorSetLayout &) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        const VulDevice &vulDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class VulDescriptorSet;
};

class VulDescriptorPool {
    public:
        class Builder {
            public:
                Builder(const VulDevice &vulDevice) : vulDevice{vulDevice} {}

                Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
                Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
                Builder &setMaxSets(uint32_t count);
                std::unique_ptr<VulDescriptorPool> build() const;

            private:
                const VulDevice &vulDevice;
                std::vector<VkDescriptorPoolSize> poolSizes{};
                uint32_t maxSets = 1000;
                VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        VulDescriptorPool(
                const VulDevice &vulDevice,
                uint32_t maxSets,
                VkDescriptorPoolCreateFlags poolFlags,
                const std::vector<VkDescriptorPoolSize> &poolSizes);
        ~VulDescriptorPool();
        VulDescriptorPool(const VulDescriptorPool &) = delete;
        VulDescriptorPool &operator=(const VulDescriptorPool &) = delete;

        bool allocateDescriptorSet(
                const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

        VkDescriptorPool &getDescriptorPoolReference() {return descriptorPool;}

        void resetPool();

    private:
        const VulDevice &vulDevice;
        VkDescriptorPool descriptorPool;

        friend class VulDescriptorSet;
};

class VulDescriptorSet{
    public:
        enum class DescriptorType{
            uniformBuffer,
            storageBuffer,
            combinedImgSampler,
            spCombinedImgSampler,
            upCombinedImgSampler,
            combinedImgImmutableSampler,
            rawImageInfo,
            storageImage,
            accelerationStructure 
        };
        struct RawImageDescriptorInfo {
            VkDescriptorType descriptorType;
            VkDescriptorImageInfo descriptorInfo;
        };
        struct Descriptor{
            DescriptorType type;
            VkShaderStageFlags stages;
            const void *content;
            uint32_t count = 1;
        };
        static std::unique_ptr<VulDescriptorSet> createDescriptorSet(const std::vector<Descriptor> &descriptors, const VulDescriptorPool &pool);

        VulDescriptorSet(std::shared_ptr<VulDescriptorSetLayout> setLayout, const VulDescriptorPool &pool);

        VulDescriptorSet(const VulDescriptorSet &) = delete;
        VulDescriptorSet &operator=(const VulDescriptorSet &) = delete;
        VulDescriptorSet(VulDescriptorSet&&) = default;

        void free();

        VulDescriptorSet &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo, uint32_t descriptorCount = 1);
        VulDescriptorSet &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo, uint32_t descriptorCount = 1);
        VulDescriptorSet &writeTlas(uint32_t binding, VkWriteDescriptorSetAccelerationStructureKHR *tlasInfo, uint32_t descriptorCount = 1);

        bool build();
        void update();

        VkDescriptorSet getSet() const {return m_set;}
        std::shared_ptr<VulDescriptorSetLayout> getLayout() const {return m_setLayout;}
        bool hasSet() const {return m_hasSet;}

        struct DescriptorInfo{
            std::vector<VkDescriptorBufferInfo> bufferInfos;
            std::vector<VkDescriptorImageInfo> imageInfos;
            std::vector<VkWriteDescriptorSetAccelerationStructureKHR> tlasInfos;
        };
        std::vector<DescriptorInfo> descriptorInfos;
        
    private:
        void overwrite();

        std::shared_ptr<VulDescriptorSetLayout> m_setLayout;
        const VulDescriptorPool &m_pool;
        std::vector<VkWriteDescriptorSet> m_writes;

        VkDescriptorSet m_set;
        bool m_hasSet = false;
};

}
