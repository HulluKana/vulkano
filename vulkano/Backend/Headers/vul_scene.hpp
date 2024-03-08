#pragma once

#include <cstdint>
#include <memory>

#include"vul_gltf_loader.hpp"
#include"vul_buffer.hpp"

class Scene
{
    public:
        Scene(vulB::VulDevice &vulDevice);

        void loadScene(std::string fileName);

        std::vector<GltfLoader::GltfLight> lights;
        std::vector<GltfLoader::GltfNode> nodes;
        std::vector<GltfLoader::GltfPrimMesh> meshes;
        std::vector<GltfLoader::Material> materials;

        std::unique_ptr<vulB::VulBuffer> vertexBuffer;
        std::unique_ptr<vulB::VulBuffer> normalBuffer;
        std::unique_ptr<vulB::VulBuffer> uvBuffer;
        std::unique_ptr<vulB::VulBuffer> indexBuffer;
        std::unique_ptr<vulB::VulBuffer> materialBuffer;
        std::unique_ptr<vulB::VulBuffer> primInfoBuffer;

        std::vector<std::shared_ptr<void>> pPushDatas;
        uint32_t pushDataSize;
        
    private:
        vulB::VulDevice &m_vulDevice; 

        struct PrimitiveInfo{
            uint32_t indexOffset;
            uint32_t vertexOffset;
            int materialIndex;
        };
};
