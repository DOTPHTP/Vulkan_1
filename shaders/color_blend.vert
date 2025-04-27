#version 450
#extension GL_KHR_vulkan_glsl : enable
layout(location = 0) out vec2 fragTexCoord;

void main() {
    // 定义全屏四边形的顶点位置和纹理坐标
    vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0), // 左下角
        vec2(-1.0,  1.0), // 左上角
        vec2( 1.0, -1.0),  // 右下角
        vec2( 1.0,  1.0) // 右上角
    );

    vec2 texCoords[4] = vec2[](
        vec2(0.0, 0.0), // 左下角
        vec2(0.0, 1.0), // 左上角
        vec2(1.0, 0.0),  // 右下角
        vec2(1.0, 1.0) // 右上角
    );

    gl_Position = vec4(positions[gl_VertexIndex%4], 0.0, 1.0);
    fragTexCoord = texCoords[gl_VertexIndex%4];
}
