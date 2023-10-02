#pragma once
 
#include "vul_device.hpp"
 
// std
#include <memory>
#include <unordered_map>
#include <vector>
 
namespace vulB {
 
class VulDescriptorSetLayout {
 public:
  class Builder {
   public:
    Builder(VulDevice &vulDevice) : vulDevice{vulDevice} {}
 
    Builder &addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count = 1);
    std::unique_ptr<VulDescriptorSetLayout> build() const;
 
   private:
    VulDevice &vulDevice;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
  };
 
  VulDescriptorSetLayout(
      VulDevice &vulDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
  ~VulDescriptorSetLayout();
  VulDescriptorSetLayout(const VulDescriptorSetLayout &) = delete;
  VulDescriptorSetLayout &operator=(const VulDescriptorSetLayout &) = delete;
 
  VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
 
 private:
  VulDevice &vulDevice;
  VkDescriptorSetLayout descriptorSetLayout;
  std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
 
  friend class VulDescriptorWriter;
};
 
class VulDescriptorPool {
 public:
  class Builder {
   public:
    Builder(VulDevice &vulDevice) : vulDevice{vulDevice} {}
 
    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
    Builder &setMaxSets(uint32_t count);
    std::unique_ptr<VulDescriptorPool> build() const;
 
   private:
    VulDevice &vulDevice;
    std::vector<VkDescriptorPoolSize> poolSizes{};
    uint32_t maxSets = 1000;
    VkDescriptorPoolCreateFlags poolFlags = 0;
  };
 
  VulDescriptorPool(
      VulDevice &vulDevice,
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
  VulDevice &vulDevice;
  VkDescriptorPool descriptorPool;
 
  friend class VulDescriptorWriter;
};
 
class VulDescriptorWriter {
 public:
  VulDescriptorWriter(VulDescriptorSetLayout &setLayout, VulDescriptorPool &pool);
 
  VulDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
  VulDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);
 
  bool build(VkDescriptorSet &set);
  void overwrite(VkDescriptorSet &set);
 
 private:
  VulDescriptorSetLayout &setLayout;
  VulDescriptorPool &pool;
  std::vector<VkWriteDescriptorSet> writes;
};
 
}