#include "../Headers/vul_descriptors.hpp"
 
// std
#include <cassert>
#include <stdexcept>
 
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
 
  // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
  // a new pool whenever an old pool fills up. But this is beyond our current scope
  if (vkAllocateDescriptorSets(vulDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
    return false;
  }
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
 
VulDescriptorWriter::VulDescriptorWriter(VulDescriptorSetLayout &setLayout, VulDescriptorPool &pool)
    : setLayout{setLayout}, pool{pool} {}
 
VulDescriptorWriter &VulDescriptorWriter::writeBuffer(
    uint32_t binding, VkDescriptorBufferInfo *bufferInfo) {
  assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
 
  auto &bindingDescription = setLayout.bindings[binding];
 
  assert(
      bindingDescription.descriptorCount == 1 &&
      "Binding single descriptor info, but binding expects multiple");
 
  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = bindingDescription.descriptorType;
  write.dstBinding = binding;
  write.pBufferInfo = bufferInfo;
  write.descriptorCount = 1;
 
  writes.push_back(write);
  return *this;
}
 
VulDescriptorWriter &VulDescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo, uint32_t descriptorCount) {
  assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
 
  auto &bindingDescription = setLayout.bindings[binding];
 
  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = bindingDescription.descriptorType;
  write.dstBinding = binding;
  write.pImageInfo = imageInfo;
  write.descriptorCount = descriptorCount;
 
  writes.push_back(write);
  return *this;
}
 
bool VulDescriptorWriter::build(VkDescriptorSet &set) {
  bool success = pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
  if (!success) {
    return false;
  }
  overwrite(set);
  return true;
}
 
void VulDescriptorWriter::overwrite(VkDescriptorSet &set) {
  for (auto &write : writes) {
    write.dstSet = set;
  }
  vkUpdateDescriptorSets(pool.vulDevice.device(), writes.size(), writes.data(), 0, nullptr);
}
 
}