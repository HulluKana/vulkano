#include"../Headers/vul_model.hpp"
#include"../Headers/vul_utils.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include<tiny_obj_loader.h>
#include<glm/gtx/hash.hpp>

#include<cassert>
#include<cstring>
#include<iostream>
#include<unordered_map>

namespace std {
template <>
struct hash<vulB::VulModel::Vertex> {
    size_t operator()(vulB::VulModel::Vertex const &vertex) const {
        size_t seed = 0;
        vulB::hashCombine(seed, vertex.pos, vertex.normal, vertex.uv);
        return seed;
    }
};

}

namespace vulB {

VulModel::VulModel(VulDevice &device, const VulModel::Builder &builder) : vulDevice{device} 
{
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
}

VulModel::~VulModel()
{

}

std::unique_ptr<VulModel> VulModel::createModelFromFile(VulDevice &device, const std::string &filepath)
{
    Builder builder{};
    builder.loadModel(filepath);

    return std::make_unique<VulModel>(device, builder);
}

std::unique_ptr<VulModel> VulModel::createScreenModelFromData(VulDevice &device, const std::vector<glm::vec2> &corners)
{
    Builder builder{};
    builder.createModel(corners);

    return std::make_unique<VulModel>(device, builder);
}

void VulModel::createVertexBuffers(const std::vector<Vertex> &vertices)
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
    uint32_t vertexSize = sizeof(vertices[0]);

    VulBuffer stagingBuffer{vulDevice, vertexSize, vertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    stagingBuffer.map();
    stagingBuffer.writeToBuffer((void *)vertices.data());

    vertexBuffer = std::make_unique<VulBuffer>(vulDevice, vertexSize, vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vulDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void VulModel::createIndexBuffers(const std::vector<uint32_t> &indices)
{
    indexCount = static_cast<uint32_t>(indices.size());
    
    hasIndexBuffer = indexCount > 0;
    if (!hasIndexBuffer) return;

    assert(indexCount >= 3 && "Index count must be 0 or at least 3");
    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
    uint32_t indexSize = sizeof(indices[0]);
    VulBuffer stagingBuffer(vulDevice, indexSize, indexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.map();
    stagingBuffer.writeToBuffer((void *)indices.data());

    indexBuffer = std::make_unique<VulBuffer>(vulDevice, indexSize, indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vulDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}

void VulModel::draw(VkCommandBuffer commandBuffer)
{
    if (hasIndexBuffer) vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    else vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0); 
}

void VulModel::bind(VkCommandBuffer commandBuffer)
{
    VkBuffer buffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (hasIndexBuffer) vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

std::vector<VkVertexInputBindingDescription> VulModel::Vertex::getBindingDescriptions()
{
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> VulModel::Vertex::getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    return attributeDescriptions;    
}

void VulModel::Builder::loadModel(const std::string &filepath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())){
        throw std::runtime_error(warn + err);
    }

    vertices.clear();
    indices.clear();

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    for (const tinyobj::shape_t &shape : shapes){
        for (const tinyobj::index_t &index : shape.mesh.indices){
            Vertex vertex{};

            if (index.vertex_index >= 0){
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
            }

            if (index.normal_index >= 0){
                vertex.normal = {
                    attrib.normals[3 * index.normal_index],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index+ 2]
                };
            }

            if (index.texcoord_index >= 0){
                vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index],
                    attrib.texcoords[2 * index.texcoord_index + 1],
                };
            }

            if (uniqueVertices.count(vertex) == 0){
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void VulModel::Builder::createModel(const std::vector<glm::vec2> &corners)
{
    vertices.clear();
    indices.clear();

    for (const glm::vec2 &corner : corners){
        Vertex vertex{};
        vertex.pos = glm::vec3(corner, 0.5f);
        vertex.normal = glm::vec3(1.0f);
        vertex.uv = corner;
        vertices.push_back(vertex);
    }
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);
}

}