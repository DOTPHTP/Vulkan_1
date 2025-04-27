#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D colorAttachment1; // 第一个颜色附件
layout(binding = 1) uniform sampler2D colorAttachment2; // 第二个颜色附件

void main() {
    // 使用 fragTexCoord 采样两个颜色附件
    vec4 color1 = texture(colorAttachment1, fragTexCoord);
    vec4 color2 = texture(colorAttachment2, fragTexCoord);

    // 混合两个颜色附件的颜色
    outColor = vec4(color1.r, color2.g,color2.b, 1); // 简单的线性混合
}
