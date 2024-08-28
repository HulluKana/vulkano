#ifndef VUL_MESHLET_SCENE_H
#define VUL_MESHLET_SCENE_H
#include "vul_scene.hpp"
#ifdef __cplusplus

#include <vul_command_pool.hpp>
#include <vul_gltf_loader.hpp>

namespace vul {

class VulMeshletScene {
    public:
        struct WantedBuffers {
            bool vertex = true;
            bool normal = true;
            bool tangent = true;
            bool uv = true;
            bool vertIdxs = true;
            bool triIdxs = true;
            bool material = true;
            bool meshlets = true;
            bool meshletBounds = true;
            bool meshes = true;
            bool indirectDrawCommands = true;
        };

        void loadGltfSync(const std::string &fileName, const std::string &textureDir, uint32_t maxTriangles, uint32_t maxVertices,
                uint32_t maxMeshletsPerWorkgroup, const WantedBuffers &wantedBuffers, VulCmdPool &cmdPool,
                const VulDevice &vulDevice);
        std::unique_ptr<GltfLoader::AsyncImageLoadingInfo> loadGltfAsync(const std::string &fileName, const std::string &textureDir,
                uint32_t maxTriangles, uint32_t maxVertices, uint32_t maxMeshletsPerWorkgroup, uint32_t asyncMipLoadCount,
                const WantedBuffers &wantedBuffers, VulCmdPool &cmdPool, VulCmdPool &transferCmdPool, VulCmdPool &dstCmdPool,
                const VulDevice &vulDevice);

        using vec3 = glm::vec3;
        using vec4 = glm::vec4;
        using mat4 = glm::mat4;
        using uint = uint32_t;
        using i8vec3 = glm::vec<3, int8_t>;
        #endif // __cplusplus

        struct MeshInfo {
            mat4 modelMatrix;
            uint meshletOffset;
            uint meshletCount;
            uint matIdx;
        };

        struct Meshlet {
            uint triangleOffset;
            uint vertexOffset;
            uint16_t vertexCount;
            uint16_t triangleCount;
        };

        struct MeshletBounds {
            vec3 center;
            float radius;
            i8vec3 coneAxis;
            int8_t coneCutoff;
        };

        #ifdef __cplusplus

        std::vector<MeshInfo> meshes;
        std::vector<Meshlet> meshlets;
        std::vector<MeshletBounds> meshletBounds;
        std::vector<VkDrawMeshTasksIndirectCommandEXT> indirectDrawCommands;
        std::vector<GltfLoader::GltfLight> lights;
        std::vector<GltfLoader::Material> materials;
        std::vector<std::shared_ptr<VulImage>> images;

        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec4> tangents;
        std::vector<glm::vec2> uvs;
        std::vector<uint32_t> vertIndices;
        std::vector<uint8_t> triIndices;

        std::unique_ptr<VulBuffer> vertexBuffer;
        std::unique_ptr<VulBuffer> normalBuffer;
        std::unique_ptr<VulBuffer> tangentBuffer;
        std::unique_ptr<VulBuffer> uvBuffer;
        std::unique_ptr<VulBuffer> vertIndexBuffer;
        std::unique_ptr<VulBuffer> triIndexBuffer;
        std::unique_ptr<VulBuffer> materialBuffer;
        std::unique_ptr<VulBuffer> meshletBuffer;
        std::unique_ptr<VulBuffer> meshletBoundsBuffer;
        std::unique_ptr<VulBuffer> meshBuffer;
        std::unique_ptr<VulBuffer> indirectDrawCommandsBuffer;

    private:
        void createMeshletsFromScene(vul::Scene &scene, uint32_t maxTriangles, uint32_t maxVertices,
                uint32_t maxMeshletsPerWorkgroup, const WantedBuffers &wantedBuffers, VulCmdPool &cmdPool,
                const VulDevice &vulDevice);
};

}

#endif // __cplusplus
#endif // VUL_MESHLET_SCENE_H
