#version 450
#extension  GL_ARB_shader_viewport_layer_array : enable
#extension   GL_NV_viewport_array2 : enable
#extension GL_EXT_multiview : enable
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPosition;
layout(location = 4)flat out int Instanceidx;
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view[2];
    mat4 proj;
} ubo;

void main() {
    int idx = gl_ViewIndex;
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view[idx] * worldPosition;
    //gl_Position.y = -gl_Position.y; // Vulkan需要翻转Y轴
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = mat3(transpose(inverse(ubo.model))) * inNormal; // 法线转换到世界空间
    fragPosition = worldPosition.xyz; // 传递世界空间位置
    Instanceidx = idx;
}