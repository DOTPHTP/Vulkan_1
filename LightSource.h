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

    // ���ù�Դ����
    void setType(LightType newType) {
        type = newType;
    }

    // ��ȡ��Դ����
    LightType getType() const {
        return type;
    }

    // ���ù�Դλ��
    void setPosition(const glm::vec3& newPosition) {
        position = newPosition;
    }

    // ��ȡ��Դλ��
    glm::vec3 getPosition() const {
        return position;
    }

    // ���ù�Դ��ɫ
    void setColor(const glm::vec3& newColor) {
        color = newColor;
    }

    // ��ȡ��Դ��ɫ
    glm::vec3 getColor() const {
        return color;
    }

    // ���ù�Դǿ��
    void setIntensity(float newIntensity) {
        intensity = newIntensity;
    }

    // ��ȡ��Դǿ��
    float getIntensity() const {
        return intensity;
    }

    // ���ù�Դ���򣨽������ڷ����;۹�ƣ�
    void setDirection(const glm::vec3& newDirection) {
        direction = glm::normalize(newDirection);
    }

    // ��ȡ��Դ����
    glm::vec3 getDirection() const {
        return direction;
    }

    // ���þ۹�ƵĽ�׶�ǣ��������ھ۹�ƣ�
    void setCutoffAngle(float angle) {
        cutoffAngle = angle;
    }

    // ��ȡ�۹�ƵĽ�׶��
    float getCutoffAngle() const {
        return cutoffAngle;
    }

private:
    LightType type;          // ��Դ����
    glm::vec3 position;      // ��Դλ��
    glm::vec3 color;         // ��Դ��ɫ
    float intensity;         // ��Դǿ��
    glm::vec3 direction;     // ��Դ����
    float cutoffAngle;       // �۹�ƵĽ�׶��
};
