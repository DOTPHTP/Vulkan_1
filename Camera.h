#pragma once 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // 添加四元数支持
class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, float fov, float aspectRatio, float nearPlane, float farPlane)
        : position(position), target(target), up(up), fov(fov), aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane) {
        forward = glm::normalize(target - position);
        right = glm::normalize(glm::cross(forward, up));
        this->up = glm::normalize(glm::cross(right, forward));
        rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    // 平移相机
    void translate(const glm::vec3& translation) {
        position += translation.x * right + translation.y * up + translation.z * forward;
    }

    // 使用四元数绕某个轴旋转相机
    void rotate(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // 创建旋转四元数
            rotation = newRotation * rotation; // 累积旋转（注意顺序：新旋转在前）

            // 更新局部坐标系基向量
            forward = glm::normalize(newRotation * forward);
            right = glm::normalize(glm::cross(forward, up));
            up = glm::normalize(glm::cross(right, forward));
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
    // 使用四元数绕目标点旋转相机
    void rotateAroundTarget(float angle, const glm::vec3& axis) {
        if (glm::length(axis) > 0.0f) {
            glm::quat newRotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis)); // 创建旋转四元数
            rotation = newRotation * rotation; // 累积旋转
            glm::vec3 direction = position - (position + forward);
            direction = newRotation * direction; // 应用旋转到方向向量
            position = (position + forward) + direction; // 更新相机位置

            // 更新局部坐标系基向量
            forward = glm::normalize(newRotation * forward);
            right = glm::normalize(glm::cross(forward, up));
            up = glm::normalize(glm::cross(right, forward));
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
    // 获取视图矩阵
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, target, up);
    }
    
    // 获取投影矩阵
    glm::mat4 getProjectionMatrix() const {
        glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        proj[1][1] *= -1; // Vulkan 需要翻转 Y 轴,使用扩展支持
        return proj;
    }
	//这里使用平行相机，即同时平移相机和目标点
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

    // 重置相机到初始状态
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
    glm::vec3 forward;  // 前方向（目标方向）
    glm::vec3 up;       // 上方向
    glm::vec3 right;    // 右方向
    glm::quat rotation; // 相机的旋转四元数
    glm::vec3 target;   // 目标点
    float eyeOffset = 0.03f; //屏幕视差
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    // 更新方向向量
    
};
