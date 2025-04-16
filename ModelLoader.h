#pragma once
#define TINYOBJLOADER_IMPLEMENTATION
#include "Vertex.h"
#include <iostream>
#include <vector>
#include <string>
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>


class ModelLoader {
public:
    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        int materialIndex = -1;  // 使用的材质索引
    };

    struct Material {
        glm::vec3 ambientColor;   // Ka: 环境光反射系数
        glm::vec3 diffuseColor;   // Kd: 漫反射系数
        glm::vec3 specularColor;  // Ks: 镜面反射系数
        float shininess;          // Ns: 镜面高光指数
        float opacity;            // d: 透明度
        std::string name;         // 材质名称
        std::string texturePath;  // 纹理路径
    };

    // 加载单个模型
    bool loadModel(const std::string& modelPath, const std::string& materialBaseDir) {
        clear(); // 清除之前的数据
        bool ok =loadModelInternal(modelPath, materialBaseDir);
        if(ok)normalizeModel();
        return ok;
    }

    const std::vector<Mesh>& getMeshes() const { return meshes; }
    const std::vector<Material>& getMaterials() const { return materials; }

private:
    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    // 清除所有数据
    void clear() {
        meshes.clear();
        materials.clear();
    }

    // 内部加载模型函数，支持材质偏移量
    bool loadModelInternal(const std::string& modelPath, const std::string& materialBaseDir, size_t materialOffset = 0) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> tinyMaterials;
        std::string warn, err;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &tinyMaterials, &warn, &err,
            modelPath.c_str(), materialBaseDir.c_str(), true);

        if (!warn.empty()) {
            std::cout << "WARN: " << warn << std::endl;
        }

        if (!err.empty()) {
            std::cerr << "ERR: " << err << std::endl;
        }

        if (!ret) {
            return false;
        }

        // 如果没有材质数据，创建一个默认材质
        if (tinyMaterials.empty()) {
            tinyobj::material_t defaultMaterial;
            defaultMaterial.name = "default";
            defaultMaterial.ambient[0] = 0.2f;  // 默认环境光
            defaultMaterial.ambient[1] = 0.2f;
            defaultMaterial.ambient[2] = 0.2f;
            defaultMaterial.diffuse[0] = 0.8f;  // 默认漫反射
            defaultMaterial.diffuse[1] = 0.8f;
            defaultMaterial.diffuse[2] = 0.8f;
            defaultMaterial.specular[0] = 0.0f; // 默认镜面反射
            defaultMaterial.specular[1] = 0.0f;
            defaultMaterial.specular[2] = 0.0f;
            defaultMaterial.shininess = 1.0f;   // 默认高光指数
            defaultMaterial.dissolve = 1.0f;    // 默认不透明
            tinyMaterials.push_back(defaultMaterial);
        }

        size_t oldMaterialSize = materials.size();
        processMaterials(tinyMaterials, materialBaseDir);
        processMeshes(attrib, shapes, oldMaterialSize + materialOffset);

        return true;
    }

    // 处理材质，支持没有材质文件的情况
    void processMaterials(const std::vector<tinyobj::material_t>& tinyMaterials, const std::string& materialBaseDir) {
        size_t startIdx = materials.size();
        materials.resize(startIdx + tinyMaterials.size());

        for (size_t i = 0; i < tinyMaterials.size(); i++) {
            const auto& mat = tinyMaterials[i];
            Material& material = materials[startIdx + i];

            // 材质名称
            material.name = mat.name;

            // 环境光反射系数 (Ka)
            material.ambientColor = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);

            // 漫反射系数 (Kd)
            material.diffuseColor = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);

            // 镜面反射系数 (Ks)
            material.specularColor = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);

            // 镜面高光指数 (Ns)
            material.shininess = mat.shininess;

            // 透明度 (d)
            material.opacity = mat.dissolve;

            if (!mat.diffuse_texname.empty()) {
                // 尝试构建纹理路径，处理绝对路径和相对路径
                std::string texPath = mat.diffuse_texname;
                if (!std::filesystem::path(texPath).is_absolute()) {
                    texPath = materialBaseDir + "/" + texPath;
                }

                if (std::filesystem::exists(texPath)) {
                    material.texturePath = texPath;
                }
                else {
                    std::cerr << "Warning: Texture file not found: " << texPath << std::endl;
                }
            }
        }
    }

    // 处理网格，增加材质偏移量参数
    void processMeshes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, size_t materialOffset = 0) {
        for (const auto& shape : shapes) {
            std::map<int, Mesh> materialToMesh;
            size_t indexOffset = 0;

            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                int materialId = shape.mesh.material_ids[f];

                // 检查材质ID是否有效，无效则使用默认材质(0)
                if (materialId < 0) {
                    materialId = 0;
                }

                // 应用材质偏移量
                materialId += materialOffset;

                if (materialToMesh.find(materialId) == materialToMesh.end()) {
                    materialToMesh[materialId] = Mesh();
                    materialToMesh[materialId].materialIndex = materialId;
                }

                Mesh& currentMesh = materialToMesh[materialId];
                size_t fv = shape.mesh.num_face_vertices[f];

                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                    Vertex vertex{};

                    if (idx.vertex_index >= 0 && 3 * idx.vertex_index + 2 < attrib.vertices.size()) {
                        vertex.pos = {
                            attrib.vertices[3 * idx.vertex_index + 0],
                            attrib.vertices[3 * idx.vertex_index + 1],
                            attrib.vertices[3 * idx.vertex_index + 2]
                        };

                        // 使用材质颜色作为顶点颜色
                        if (materialId >= 0 && materialId < materials.size()) {
                            vertex.color = materials[materialId].diffuseColor;
                        }
                        else {
                            vertex.color = glm::vec3(1.0f);  // 默认白色
                        }
                    }

                    if (idx.normal_index >= 0 && 3 * idx.normal_index + 2 < attrib.normals.size()) {
                        vertex.normal = {
                            attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]
                        };
                    }
                    else {
                        // 没有法线则设置为默认向上法线
                        vertex.normal = { 0.0f, 1.0f, 0.0f };
                    }

                    if (idx.texcoord_index >= 0 && 2 * idx.texcoord_index + 1 < attrib.texcoords.size()) {
                        vertex.texCoord = {
                            attrib.texcoords[2 * idx.texcoord_index + 0],
                            attrib.texcoords[2 * idx.texcoord_index + 1]
                        };
                    }
                    else {
                        // 没有纹理坐标则设置为默认值
                        vertex.texCoord = { 0.0f, 0.0f };
                    }

                    currentMesh.vertices.push_back(vertex);
                    currentMesh.indices.push_back(static_cast<uint32_t>(currentMesh.vertices.size() - 1));
                }

                indexOffset += fv;
            }

            for (auto& [matId, mesh] : materialToMesh) {
                meshes.push_back(std::move(mesh));
            }
        }
    }

    void normalizeModel() {

        // 如果没有网格，直接返回
        if (meshes.empty()) {
            return;
        }

        glm::vec3 minBounds(std::numeric_limits<float>::max());
        glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

        for (const auto& mesh : meshes) {
            for (const auto& vertex : mesh.vertices) {
                minBounds = glm::min(minBounds, vertex.pos);
                maxBounds = glm::max(maxBounds, vertex.pos);
            }
        }
       
        glm::vec3 center = (minBounds + maxBounds) * 0.5f;
        glm::vec3 size = maxBounds - minBounds;
        float maxDimension = std::max(std::max(size.x, size.y), size.z);

        // 防止除零错误
        if (maxDimension <= 0.0f) {
            std::cerr << "Warning: Model has zero or negative dimension!" << std::endl;
            maxDimension = 1.0f;
        }

        float normalizeFactor = 2.0f / maxDimension;

        for (auto& mesh : meshes) {
            for (auto& vertex : mesh.vertices) {
                vertex.pos = (vertex.pos - center) * normalizeFactor;
            }
        }
    }
};