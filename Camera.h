#pragma once 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // �����Ԫ��֧��
class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, float fov, float aspectRatio, float nearPlane, float farPlane)
        : position(position), target(target), up(up), fov(fov), aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane) {
        forward = glm::normalize(target - position);
        right = glm::normalize(glm::cross(forward, up));
        this->up = glm::normalize(glm::cross(right, forward));
        rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    // ƽ�����
    void translate(const glm::vec3& translation) {
        position += translation.x * right + translation.y * up + translation.z * forward;
    }

    // ʹ����Ԫ����ĳ������ת���
    void rotate(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // ������ת��Ԫ��
            rotation = newRotation * rotation; // �ۻ���ת��ע��˳������ת��ǰ��

            // ���¾ֲ�����ϵ������
            forward = glm::normalize(newRotation * forward);
            right = glm::normalize(glm::cross(forward, up));
            up = glm::normalize(glm::cross(right, forward));
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
    // ʹ����Ԫ����Ŀ�����ת���
    void rotateAroundTarget(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // ������ת��Ԫ��
            rotation = newRotation * rotation; // �ۻ���ת
            glm::vec3 direction = position - (position + forward);
            direction = newRotation * direction; // Ӧ����ת����������
            position = (position + forward) + direction; // �������λ��

            // ���¾ֲ�����ϵ������
            forward = glm::normalize(newRotation * forward);
            right = glm::normalize(glm::cross(forward, up));
            up = glm::normalize(glm::cross(right, forward));
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
    glm::vec3 getPosition() const {
        return position;
    }
	glm::vec3 getLeftEye() const {
		return position - right * eyeOffset;
	}
	glm::vec3 getRightEye() const {
		return position + right * eyeOffset;
	}
    glm::vec3 getTarget() const {
        return target;
    }
    glm::vec3 getRight() const {
        return right;
    }

    glm::vec3 getForward() const {
        return forward;
    }
    // ��ȡ��ͼ����
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, target, up);
    }
    
    // ��ȡͶӰ����
    glm::mat4 getProjectionMatrix() const {
        glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        proj[1][1] *= -1; // Vulkan ��Ҫ��ת Y ��,ʹ����չ֧��
        return proj;
    }
	//����ʹ��ƽ���������ͬʱƽ�������Ŀ���
    glm::mat4 getLeftViewMatrix_Fixed() const {
        return glm::lookAt(
            position - right * eyeOffset,
            target - right*eyeOffset,
            up
        );
    }

    glm::mat4 getRightViewMatrix_Fixed() const {
        
        return glm::lookAt(
            position + right * eyeOffset,
            target + right * eyeOffset,
            up
        );
    }

    // �����������ʼ״̬
    void reset(glm::vec3 newPosition, glm::vec3 newTarget, glm::vec3 newUp) {
        position = newPosition;
        target = newTarget;
        forward = glm::normalize(newTarget - newPosition);
        right = glm::normalize(glm::cross(forward, newUp));
        up = glm::normalize(glm::cross(right, forward));
        rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

	float geteyeOffset() const {
		return eyeOffset;
	}

	void seteyeOffset(float offset) {
		eyeOffset = offset;
	}
	
private:
    glm::vec3 position;
    glm::vec3 forward;  // ǰ����Ŀ�귽��
    glm::vec3 up;       // �Ϸ���
    glm::vec3 right;    // �ҷ���
    glm::quat rotation; // �������ת��Ԫ��
    glm::vec3 target;   // Ŀ���
    float eyeOffset = 0.03f; //��Ļ�Ӳ�
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    // ���·�������
    
};
