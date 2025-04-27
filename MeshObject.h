#pragma once
#include "ModelLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>

class MeshObject {
public:
    // ���캯�������ܴ� ModelLoader ���ص�����Ͳ�������
    MeshObject(const std::vector<ModelLoader::Mesh>& meshes, const std::vector<ModelLoader::Material>& materials)
        : meshes(meshes), materials(materials) {}

    // ��ȡģ�͵ı任����
    // ��ȡģ�͵ı任����
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, translation); // Ӧ��ƽ��
        model *= glm::toMat4(rotation); // Ӧ����ת����Ԫ��ת��Ϊ����
        model = glm::scale(model, scaling); // Ӧ������
        return model;
    }


    // �ۼ�ƽ�Ʊ任
    void translate(const glm::vec3& translation) {
        this->translation += translation;
    }

    // �ۼ����ű任
    void scale(const glm::vec3& scaling) {
        this->scaling *= scaling;
        // ��ֹ����Ϊ���ֵ
        this->scaling = glm::max(this->scaling, glm::vec3(0.001f));
    }

    // �ۼ���ת�任����ָ������ת��
    void rotate(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // �����µ���ת��Ԫ��
            rotation = newRotation * rotation; // �ۼ���ת����Ԫ����ˣ�
        }
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
        // ��ʼ����Χ��Ϊ��ֵ
        minBounds = glm::vec3(std::numeric_limits<float>::max());
        maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
		glm::mat4 modelMatrix = getModelMatrix();
        // ������������
        for (const auto& mesh : meshes) {
            for (const auto& vertex : mesh.vertices) {
                // Ӧ��ģ�͵ı任����
                glm::vec4 transformedPos = modelMatrix * glm::vec4(vertex.pos, 1.0f);
                glm::vec3 worldPos = glm::vec3(transformedPos);

                // ���°�Χ��
                minBounds = glm::min(minBounds, worldPos);
                maxBounds = glm::max(maxBounds, worldPos);
            }
        }
    }
    void setParentIndex(int index) {
        parentIndex = index;
    }

    int getParentIndex() const {
        return parentIndex;
    }

    // ����ģ�ͱ任����Ϊ��λ����
    void resetTransform() {
        translation = glm::vec3(0.0f);
        scaling = glm::vec3(1.0f);
        rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // ��λ��Ԫ��
    }

private:
    // �任����
    glm::vec3 translation = { 0, 0, 0 }; // ƽ������
    glm::vec3 scaling = { 1, 1, 1 };     // ��������
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // ��ת��Ԫ������ʼΪ��λ��Ԫ����
    std::vector<ModelLoader::Mesh> meshes;       // ��������
    std::vector<ModelLoader::Material> materials; // ��������
    int parentIndex = -1; // �������������-1 ��ʾû�и�����
                         
};
