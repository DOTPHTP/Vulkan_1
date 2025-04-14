#pragma once
#include "ModelLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

class MeshObject {
public:
    // ���캯�������ܴ� ModelLoader ���ص�����Ͳ�������
    MeshObject(const std::vector<ModelLoader::Mesh>& meshes, const std::vector<ModelLoader::Material>& materials)
        : meshes(meshes), materials(materials), modelMatrix(1.0f) {}

    // ��ȡģ�͵ı任����
    const glm::mat4& getModelMatrix() const {
        return modelMatrix;
    }

    // ����ģ�͵ı任����
    void setModelMatrix(const glm::mat4& matrix) {
        modelMatrix = matrix;
    }

    // ƽ�Ʊ任
    void translate(const glm::vec3& translation) {
        modelMatrix = glm::translate(modelMatrix, translation);
    }

    // ���ű任
    void scale(const glm::vec3& scaling) {
        modelMatrix = glm::scale(modelMatrix, scaling);
    }

    // ��ת�任����ָ������ת��
    void rotate(float angle, const glm::vec3& axis) {
        modelMatrix = glm::rotate(modelMatrix, glm::radians(angle), axis);
    }

    // ��ȡ��������
    const std::vector<ModelLoader::Mesh>& getMeshes() const {
        return meshes;
    }

    // ��ȡ���в���
    const std::vector<ModelLoader::Material>& getMaterials() const {
        return materials;
    }

    // ��ȡָ������Ķ�������
    const std::vector<Vertex>& getVertices(size_t meshIndex) const {
        if (meshIndex >= meshes.size()) {
            throw std::out_of_range("Mesh index out of range");
        }
        return meshes[meshIndex].vertices;
    }

    // ��ȡָ���������������
    const std::vector<uint32_t>& getIndices(size_t meshIndex) const {
        if (meshIndex >= meshes.size()) {
            throw std::out_of_range("Mesh index out of range");
        }
        return meshes[meshIndex].indices;
    }

    // ��ȡģ�͵İ�Χ�У�AABB��
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

    // ����ģ�ͱ任����Ϊ��λ����
    void resetTransform() {
        modelMatrix = glm::mat4(1.0f);
    }

private:
    std::vector<ModelLoader::Mesh> meshes;       // ��������
    std::vector<ModelLoader::Material> materials; // ��������
    glm::mat4 modelMatrix;                       // ģ�ͱ任����
};
