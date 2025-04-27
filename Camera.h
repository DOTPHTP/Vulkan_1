#pragma once 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // �����Ԫ��֧��
class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, float fov, float aspectRatio, float nearPlane, float farPlane)
        : position(position), target(target), up(up), fov(fov), aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane) {
        rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        updateDirectionVectors();
    }

    // ƽ�����
    void translate(const glm::vec3& translation) {
        position += translation;
        target += translation;
        updateDirectionVectors();
    }

    // ʹ����Ԫ����ĳ������ת���
    void rotate(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // ������ת��Ԫ��
            rotation = newRotation * rotation; // �ۻ���ת��ע��˳������ת��ǰ��
            updateDirectionVectors();
        }
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
    // ʹ����Ԫ����Ŀ�����ת���
    void rotateAroundTarget(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // ������ת��Ԫ��
            rotation = newRotation * rotation; // �ۻ���ת
            glm::vec3 direction = position - target;
            direction = newRotation * direction; // Ӧ����ת����������
            position = target + direction; // �������λ��
            updateDirectionVectors();
        }
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
    glm::mat4 getLeftViewMatrix() const {
        return glm::lookAt(position - eyeOffset*right,
            target, up);
    }

    glm::mat4 getRightViewMatrix() const {
        return glm::lookAt(position + eyeOffset*right,
            target, up);
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
    glm::quat rotation; // �������ת��Ԫ��
    float eyeOffset = 0.08f; // ͫ�׼��
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    // ���·�������
    void updateDirectionVectors() {
        glm::vec3 direction = glm::normalize(target - position);
        direction = rotation * glm::vec3(0.0f, 0.0f, -1.0f); // Ӧ����ת��ǰ������
        right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f))); // ����������
        up = glm::normalize(glm::cross(right, direction)); // ����������
    }
};
