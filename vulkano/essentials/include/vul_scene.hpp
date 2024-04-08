#pragma once

#include <cstdint>
#include <memory>

#include"vul_gltf_loader.hpp"
#include"vul_buffer.hpp"

namespace vul
{

class Scene
{
    public:
        Scene(vulB::VulDevice &vulDevice);

        void loadScene(std::string fileName);

        std::vector<vulB::GltfLoader::GltfLight> lights;
        std::vector<vulB::GltfLoader::GltfNode> nodes;
        std::vector<vulB::GltfLoader::GltfPrimMesh> meshes;
        std::vector<vulB::GltfLoader::Material> materials;
        std::vector<std::shared_ptr<VulImage>> images;

        std::unique_ptr<vulB::VulBuffer> vertexBuffer;
        std::unique_ptr<vulB::VulBuffer> normalBuffer;
        std::unique_ptr<vulB::VulBuffer> uvBuffer;
        std::unique_ptr<vulB::VulBuffer> indexBuffer;
        std::unique_ptr<vulB::VulBuffer> materialBuffer;
    private:
        vulB::VulDevice &m_vulDevice; 
};

}
