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
        : meshes(meshes), materials(materials) {}

    // 获取模型的变换矩阵
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, translation); // 应用平移
        model = glm::rotate(model, glm::radians(rotationAngle), rotationAxis); // 应用旋转
        model = glm::scale(model, scaling); // 应用缩放
        return model;
    }

    // 累加平移变换
    void translate(const glm::vec3& translation) {
        this->translation += translation;
    }

    // 累加缩放变换
    void scale(const glm::vec3& scaling) {
        this->scaling *= scaling;
        // 防止缩放为零或负值
        this->scaling = glm::max(this->scaling, glm::vec3(0.001f));
    }

    // 累加旋转变换（绕指定轴旋转）
    void rotate(float angle, const glm::vec3& axis) {
        // 累加旋转角度
        this->rotationAngle += angle;

        // 更新旋转轴（保持单位化）
        if (glm::length(axis) > 0.0f) {
            this->rotationAxis = glm::normalize(axis);
        }
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
        // 初始化包围盒为极值
        minBounds = glm::vec3(std::numeric_limits<float>::max());
        maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
		glm::mat4 modelMatrix = getModelMatrix();
        // 遍历所有网格
        for (const auto& mesh : meshes) {
            for (const auto& vertex : mesh.vertices) {
                // 应用模型的变换矩阵
                glm::vec4 transformedPos = modelMatrix * glm::vec4(vertex.pos, 1.0f);
                glm::vec3 worldPos = glm::vec3(transformedPos);

                // 更新包围盒
                minBounds = glm::min(minBounds, worldPos);
                maxBounds = glm::max(maxBounds, worldPos);
            }
        }
    }

    // 重置模型变换矩阵为单位矩阵
    void resetTransform() {
        translation = glm::vec3(0.0f);
        scaling = glm::vec3(1.0f);
        rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        rotationAngle = 0.0f;
    }

private:
    // 变换参数
    glm::vec3 translation; // 平移向量
    glm::vec3 scaling;     // 缩放向量
    glm::vec3 rotationAxis; // 旋转轴
    float rotationAngle;   // 旋转角度（以度为单位）
    std::vector<ModelLoader::Mesh> meshes;       // 网格数据
    std::vector<ModelLoader::Material> materials; // 材质数据
                         
};
