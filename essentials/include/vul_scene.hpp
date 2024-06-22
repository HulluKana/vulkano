#ifndef VUL_SCENE_H
#define VUL_SCENE_H

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

        struct Cube {
            glm::vec3 centerPos;
            glm::vec3 dimensions;
            uint32_t matIdx;
        };
        struct Sphere {
            glm::vec3 centerPos;
            float radius;
            uint32_t subdivisionCount;
            uint32_t matIdx;
        };
        struct WantedBuffers {
            bool vertex = true;
            bool index = true;
            bool normal = true;
            bool tangent = true;
            bool uv = true;
            bool material = true;
            bool primInfo = true;
        };
        void loadCubes(const std::vector<Cube> &cubes, const std::vector<vulB::GltfLoader::Material> &mats, WantedBuffers wantedBuffers);
        void loadSpheres(const std::vector<Sphere> &spheres, const std::vector<vulB::GltfLoader::Material> &mats, WantedBuffers wantedBuffers);
        void loadScene(const std::string &fileName, std::string textureDirectory, WantedBuffers wantedBuffers);

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
        std::unique_ptr<vulB::VulBuffer> primInfoBuffer;

        using vec2 = glm::vec2;
        using vec3 = glm::vec3;
        using vec4 = glm::vec4;
        using mat4 = glm::mat4;
        using uint = uint32_t;
        #endif // __cplusplus

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

        struct PrimInfo {
            mat4 transformMatrix;
            mat4 normalMatrix;
            uint firstIndex;
            uint vertexOffset;
            int materialIndex;
            int padding;
        };

        #ifdef __cplusplus

    private:
        vulB::VulDevice &m_vulDevice; 

        void createBuffers(const std::vector<uint32_t> &indices, const std::vector<glm::vec3> &vertices,
                const std::vector<glm::vec3> &normals, const std::vector<glm::vec4> &tangents, const std::vector<glm::vec2> &uvs,
                const std::vector<vulB::GltfLoader::Material> &mats, const std::vector<vulB::GltfLoader::GltfNode> &nods, WantedBuffers wantedBuffers);
};

}

#endif // __cplusplus
#endif // VUL_SCENE_H
