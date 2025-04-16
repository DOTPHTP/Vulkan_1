#pragma once 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, float fov, float aspectRatio, float nearPlane, float farPlane)
        : position(position), target(target), up(up), fov(fov), aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane) {}

    // ƽ�����
    void translate(const glm::vec3& translation) {
        position += translation;
        target += translation;
        updateDirectionVectors();
    }

    // ��ĳ������ת���
    void rotate(float angle, const glm::vec3& axis) {
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);
        glm::vec4 newPosition = rotationMatrix * glm::vec4(position - target, 1.0f);
        position = glm::vec3(newPosition) + target;
        updateDirectionVectors();
    }

    // �����ӽ�
    void setFov(float newFov) {
        fov = newFov;
    }

    // ���ÿ�߱�
    void setAspectRatio(float newAspectRatio) {
        aspectRatio = newAspectRatio;
    }
    // ��ȡ�ҷ�������
    glm::vec3 getRight() const {
        return right;
    }
    // ��Ŀ�����ת���
    void rotateAroundTarget(float angle, const glm::vec3& axis) {
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);
        glm::vec4 newPosition = rotationMatrix * glm::vec4(position - target, 1.0f);
        position = glm::vec3(newPosition) + target;
        updateDirectionVectors();
    }
    // ��������������ӽǣ�
    void zoom(float zoomFactor) {
        fov -= zoomFactor;
        if (fov < 1.0f) fov = 1.0f; // ������С�ӽ�
        if (fov > 120.0f) fov = 120.0f; // ��������ӽ�
    }
    // ��ȡ�Ϸ�������
    glm::vec3 getUp() const {
        return up;
    }

    // ��ȡ��ͼ����
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, target, up);
    }

    // ��ȡͶӰ����
    glm::mat4 getProjectionMatrix() const {
        glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        //proj[1][1] *= -1; // Vulkan ��Ҫ��ת Y ��,ʹ����չ֧��
        return proj;
    }
	glm::vec3 getPosition() const {
		return position;
	}
    // �����������ʼ״̬
    void reset(glm::vec3 newPosition, glm::vec3 newTarget, glm::vec3 newUp) {
        position = newPosition;
        target = newTarget;
        up = glm::normalize(newUp);
        updateDirectionVectors();
    }
private:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    glm::vec3 right;    // ����ҷ���
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    // ���·�������
    void updateDirectionVectors() {
        glm::vec3 direction = glm::normalize(target - position);
        right = glm::normalize(glm::cross(direction, up));
        up = glm::normalize(glm::cross(right, direction));
    }
};
