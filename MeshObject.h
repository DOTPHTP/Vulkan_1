#pragma once
#include "ModelLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

class MeshObject {
public:
    // 构造函数，接受从 ModelLoader 加载的网格和材质数据
    MeshObject(const std::vector<ModelLoader::Mesh>& meshes, const std::vector<ModelLoader::Material>& materials)
        : meshes(meshes), materials(materials), modelMatrix(1.0f) {}

    // 获取模型的变换矩阵
    const glm::mat4& getModelMatrix() const {
        return modelMatrix;
    }

    // 设置模型的变换矩阵
    void setModelMatrix(const glm::mat4& matrix) {
        modelMatrix = matrix;
    }

    // 平移变换
    void translate(const glm::vec3& translation) {
        modelMatrix = glm::translate(modelMatrix, translation);
    }

    // 缩放变换
    void scale(const glm::vec3& scaling) {
        modelMatrix = glm::scale(modelMatrix, scaling);
    }

    // 旋转变换（绕指定轴旋转）
    void rotate(float angle, const glm::vec3& axis) {
        modelMatrix = glm::rotate(modelMatrix, glm::radians(angle), axis);
    }

    // 获取所有网格
    const std::vector<ModelLoader::Mesh>& getMeshes() const {
        return meshes;
    }

    // 获取所有材质
    const std::vector<ModelLoader::Material>& getMaterials() const {
        return materials;
    }

    // 获取指定网格的顶点数据
    const std::vector<Vertex>& getVertices(size_t meshIndex) const {
        if (meshIndex >= meshes.size()) {
            throw std::out_of_range("Mesh index out of range");
        }
        return meshes[meshIndex].vertices;
    }

    // 获取指定网格的索引数据
    const std::vector<uint32_t>& getIndices(size_t meshIndex) const {
        if (meshIndex >= meshes.size()) {
            throw std::out_of_range("Mesh index out of range");
        }
        return meshes[meshIndex].indices;
    }

    // 获取模型的包围盒（AABB）
    void getBoundingBox(glm::vec3& minBounds, glm::vec3& maxBounds) const {
        minBounds = glm::vec3(std::numeric_limits<float>::max());
        maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

        for (const auto& mesh : meshes) {
            for (const auto& vertex : mesh.vertices) {
                minBounds = glm::min(minBounds, vertex.pos);
                maxBounds = glm::max(maxBounds, vertex.pos);
            }
        }
    }

    // 重置模型变换矩阵为单位矩阵
    void resetTransform() {
        modelMatrix = glm::mat4(1.0f);
    }

private:
    std::vector<ModelLoader::Mesh> meshes;       // 网格数据
    std::vector<ModelLoader::Material> materials; // 材质数据
    glm::mat4 modelMatrix;                       // 模型变换矩阵
};
