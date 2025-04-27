#pragma once 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // 添加四元数支持
class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, float fov, float aspectRatio, float nearPlane, float farPlane)
        : position(position), target(target), up(up), fov(fov), aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane) {
        rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        updateDirectionVectors();
    }

    // 平移相机
    void translate(const glm::vec3& translation) {
        position += translation;
        target += translation;
        updateDirectionVectors();
    }

    // 使用四元数绕某个轴旋转相机
    void rotate(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // 创建旋转四元数
            rotation = newRotation * rotation; // 累积旋转（注意顺序：新旋转在前）
            updateDirectionVectors();
        }
    }

    // 设置视角
    void setFov(float newFov) {
        fov = newFov;
    }

    // 设置宽高比
    void setAspectRatio(float newAspectRatio) {
        aspectRatio = newAspectRatio;
    }
    // 获取右方向向量
    glm::vec3 getRight() const {
        return right;
    }
    // 使用四元数绕目标点旋转相机
    void rotateAroundTarget(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // 创建旋转四元数
            rotation = newRotation * rotation; // 累积旋转
            glm::vec3 direction = position - target;
            direction = newRotation * direction; // 应用旋转到方向向量
            position = target + direction; // 更新相机位置
            updateDirectionVectors();
        }
    }
    // 缩放相机（调整视角）
    void zoom(float zoomFactor) {
        fov -= zoomFactor;
        if (fov < 1.0f) fov = 1.0f; // 限制最小视角
        if (fov > 120.0f) fov = 120.0f; // 限制最大视角
    }
    // 获取上方向向量
    glm::vec3 getUp() const {
        return up;
    }

    // 获取视图矩阵
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

    // 获取投影矩阵
    glm::mat4 getProjectionMatrix() const {
        glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        //proj[1][1] *= -1; // Vulkan 需要翻转 Y 轴,使用扩展支持
        return proj;
    }
	glm::vec3 getPosition() const {
		return position;
	}
    // 重置相机到初始状态
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
    glm::vec3 right;    // 相机右方向
    glm::quat rotation; // 相机的旋转四元数
    float eyeOffset = 0.08f; // 瞳孔间距
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    // 更新方向向量
    void updateDirectionVectors() {
        glm::vec3 direction = glm::normalize(target - position);
        direction = rotation * glm::vec3(0.0f, 0.0f, -1.0f); // 应用旋转到前向向量
        right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f))); // 计算右向量
        up = glm::normalize(glm::cross(right, direction)); // 计算上向量
    }
};
