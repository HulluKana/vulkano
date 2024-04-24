#pragma once

#include "vul_device.hpp"
#include"vul_image.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include<tiny_gltf.h>

namespace vulB
{

class GltfLoader
{
    public:
        enum class GltfAttributes : uint8_t{
                NoAttribs = 0,
                Position = 1,
                Normal = 2,
                Tangent = 4,
                TexCoord = 8,
                Color = 16,
                All = 0xff
        };
        static inline GltfAttributes gltfAttribOr(GltfAttributes lhs, GltfAttributes rhs)
        {
            return static_cast<GltfAttributes>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
        }
        static inline GltfAttributes gltfAttribAnd(GltfAttributes lhs, GltfAttributes rhs)
        {
            return static_cast<GltfAttributes>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
        }
        struct Material{
            glm::vec4 colorFactor = glm::vec4(1.0f);
            int colorTextureIndex = -1;
            float roughness = 1.0f;
            float metalliness = 0.0f;
            int roughnessMetallinessTextureIndex = -1;
            int normalTextureIndex = -1;
            glm::vec3 emissiveFactor = glm::vec3(0.0f);

            // Stuff added by extensions
            float emissionStrength = 0.0f;
            float ior = 1.5f;

            std::string name;
        };
        struct GltfPrimMesh{
            uint32_t firstIndex = 0;
            uint32_t indexCount = 0;
            uint32_t vertexOffset = 0;
            uint32_t vertexCount = 0;
            int materialIndex = 0;

            glm::vec3 posMin = glm::vec3(0.0f);
            glm::vec3 posMax = glm::vec3(0.0f);

            std::string name;
        };
        struct GltfNode{
            glm::mat4 worldMatrix{1.0f};
            glm::vec3 position{0.0f};
            int primMesh = 0;

            std::string name;
        };
        struct GltfLight{
            glm::vec3 position{0.0f};
            glm::vec3 color{0.0f};
            double intensity = 0.0;

            std::string name;
        };

        std::vector<uint32_t> indices;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec4> tangents;
        std::vector<glm::vec2> uvCoords;
        std::vector<glm::vec4> colors;

        std::vector<GltfLight> lights;
        std::vector<GltfNode> nodes;
        std::vector<GltfPrimMesh> primMeshes;
        std::vector<Material> materials;
        std::vector<std::shared_ptr<vul::VulImage>> images;

        void importMaterials(const tinygltf::Model &model);
        void importTextures(const tinygltf::Model &model, VulDevice &device);
        void importDrawableNodes(const tinygltf::Model &model, GltfAttributes requestedAttributes);

    private:
        void processMesh(   const tinygltf::Model &model, const tinygltf::Primitive &mesh, 
                            GltfAttributes requestedAttributes, const std::string &name);
        void processNode(const tinygltf::Model &model, int nodeIdx, const glm::vec3 &parentPos, const glm::quat &parentRot);


        float getFloat(const tinygltf::Value &value, const std::string &name);

        template<class T>
        void copyAccessorData(  std::vector<T> &outData, size_t outFirstElement, const tinygltf::Model &model, 
                                const tinygltf::Accessor &accessor, size_t accessorFirstElement, size_t numElementsToCopy);
        template<class T>
        bool getAccessorData(const tinygltf::Model &model, const tinygltf::Accessor &accessor, std::vector<T> &attribVec);
        template <typename T>
        bool getAttribute(const tinygltf::Model &model, const tinygltf::Primitive &primitive, std::vector<T> &attribVec, const std::string &attribName);
        
        std::unordered_map<int, std::vector<uint32_t>> m_meshToPrimMesh;
        std::unordered_map<std::string, GltfPrimMesh> m_cachePrimMesh;
};

}
