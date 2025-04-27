#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D colorAttachment1; // ��һ����ɫ����
layout(binding = 1) uniform sampler2D colorAttachment2; // �ڶ�����ɫ����

void main() {
    // ʹ�� fragTexCoord ����������ɫ����
    vec4 color1 = texture(colorAttachment1, fragTexCoord);
    vec4 color2 = texture(colorAttachment2, fragTexCoord);

    // ���������ɫ��������ɫ
    outColor = vec4(color1.r, color2.g,color2.b, 1); // �򵥵����Ի��
}
