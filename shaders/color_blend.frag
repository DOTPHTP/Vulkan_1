#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4)flat in int Instanceidx;
layout(binding = 1) uniform sampler2D colorAttachment1; // ��һ����ɫ����
layout(binding = 2) uniform sampler2D colorAttachment2; // �ڶ�����ɫ����

void main() {
    vec4 color1 = texture(colorAttachment1, gl_FragCoord.xy / vec2(textureSize(colorAttachment1, 0)));
    vec4 color2 = texture(colorAttachment2, gl_FragCoord.xy / vec2(textureSize(colorAttachment2, 0)));

    // ���������ɫ��������ɫ
    outColor =vec4(color1.r, color2.g,color2.b, 1); // �򵥵����Ի��
}
