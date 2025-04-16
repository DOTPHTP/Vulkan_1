#pragma once
#include <glm/glm.hpp>
#include <string>

class LightSource {
public:
    enum class LightType {
        Directional,
        Point,
        Spot
    };

    LightSource(LightType type, const glm::vec3& position, const glm::vec3& color, float intensity)
        : type(type), position(position), color(color), intensity(intensity), direction(glm::vec3(0.0f, -1.0f, 0.0f)), cutoffAngle(45.0f) {}

    // 设置光源类型
    void setType(LightType newType) {
        type = newType;
    }

    // 获取光源类型
    LightType getType() const {
        return type;
    }

    // 设置光源位置
    void setPosition(const glm::vec3& newPosition) {
        position = newPosition;
    }

    // 获取光源位置
    glm::vec3 getPosition() const {
        return position;
    }

    // 设置光源颜色
    void setColor(const glm::vec3& newColor) {
        color = newColor;
    }

    // 获取光源颜色
    glm::vec3 getColor() const {
        return color;
    }

    // 设置光源强度
    void setIntensity(float newIntensity) {
        intensity = newIntensity;
    }

    // 获取光源强度
    float getIntensity() const {
        return intensity;
    }

    // 设置光源方向（仅适用于方向光和聚光灯）
    void setDirection(const glm::vec3& newDirection) {
        direction = glm::normalize(newDirection);
    }

    // 获取光源方向
    glm::vec3 getDirection() const {
        return direction;
    }

    // 设置聚光灯的截锥角（仅适用于聚光灯）
    void setCutoffAngle(float angle) {
        cutoffAngle = angle;
    }

    // 获取聚光灯的截锥角
    float getCutoffAngle() const {
        return cutoffAngle;
    }

private:
    LightType type;          // 光源类型
    glm::vec3 position;      // 光源位置
    glm::vec3 color;         // 光源颜色
    float intensity;         // 光源强度
    glm::vec3 direction;     // 光源方向
    float cutoffAngle;       // 聚光灯的截锥角
};
