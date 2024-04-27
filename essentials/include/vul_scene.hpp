#ifndef VUL_SCENE
#define VUL_SCENE

#ifdef __cplusplus
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
        std::unique_ptr<vulB::VulBuffer> tangentBuffer;
        std::unique_ptr<vulB::VulBuffer> uvBuffer;
        std::unique_ptr<vulB::VulBuffer> indexBuffer;
        std::unique_ptr<vulB::VulBuffer> materialBuffer;

        using vec2 = glm::vec2;
        using vec3 = glm::vec3;
        using vec4 = glm::vec4;
        using mat4 = glm::mat4;
        using uint = uint32_t;
        #endif

        struct PackedMaterial{
            vec4 colorFactor;
            vec4 emissiveFactor;
            float roughness;
            float metalliness;
            int colorTextureIndex;
            int normalTextureIndex;
            int roughnessMetallicTextureIndex;
            int padding1;
            int padding2;
            int padding3;
        };

        #ifdef __cplusplus

    private:
        vulB::VulDevice &m_vulDevice; 
};

}

#endif
#endif
