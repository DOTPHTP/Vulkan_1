#version 450
#extension GL_EXT_multiview : enable
struct Material {
	vec4 ambientColor;   // Ka: 环境光反射系数
	vec4 diffuseColor;   // Kd: 漫反射系数
	vec4 specularColor;  // Ks: 镜面反射系数
	float shininess;          // Ns: 镜面高光指数
	float opacity;
};
struct Light {
	vec4 lightPosition; // 光源位置
	vec4 lightColor;    // 光源颜色
	float lightIntensity;    // 光源强度
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4) flat in int Instanceidx;
layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler;

// 将 eye_position 包装到 uniform block 中
layout(binding = 2) uniform EyePositionBlock {
    vec3 eye_position[2];
};

// 将 Material 包装到 uniform block 中
layout(binding = 3) uniform MaterialBlock {
    Material material;
};

// 将 Light 包装到 uniform block 中
layout(binding = 4) uniform LightBlock {
    Light light;
};
void main() {
   // 法线向量归一化
    vec3 N = normalize(fragNormal);

    // 光源方向向量
    vec3 L = normalize(vec3(light.lightPosition) - fragPosition);

    // 视线方向向量
    vec3 V = normalize(eye_position[gl_ViewIndex] - fragPosition);

    // 半角向量
    vec3 H = normalize(L + V);

    // 距离衰减
    float r = length(vec3(light.lightPosition) - fragPosition);
    float attenuation = light.lightIntensity / (r * r);

    // 环境光分量
    vec4 ambient = material.ambientColor * light.lightColor * attenuation;

    // 漫反射分量
    float NdotL = max(dot(N, L), 0.0);
    vec4 diffuse = material.diffuseColor * light.lightColor * attenuation * NdotL;

    // 镜面反射分量（Blinn-Phong 使用半角向量）
    float NdotH = max(dot(N, H), 0.0);
    vec4 specular = material.specularColor * light.lightColor * attenuation * pow(NdotH, material.shininess);

    // 纹理采样
    vec4 texColor = texture(texSampler, fragTexCoord);

    // 最终颜色计算
    vec4 color = ambient + diffuse + specular;
    color *= texColor;

    outColor = vec4(color.rgb, material.opacity);
    
}