#version 450
#extension GL_KHR_vulkan_glsl : enable
layout(location = 0) out vec2 fragTexCoord;

void main() {
    // ����ȫ���ı��εĶ���λ�ú���������
    vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0), // ���½�
        vec2(-1.0,  1.0), // ���Ͻ�
        vec2( 1.0, -1.0),  // ���½�
        vec2( 1.0,  1.0) // ���Ͻ�
    );

    vec2 texCoords[4] = vec2[](
        vec2(0.0, 0.0), // ���½�
        vec2(0.0, 1.0), // ���Ͻ�
        vec2(1.0, 0.0),  // ���½�
        vec2(1.0, 1.0) // ���Ͻ�
    );

    gl_Position = vec4(positions[gl_VertexIndex%4], 0.0, 1.0);
    fragTexCoord = texCoords[gl_VertexIndex%4];
}
