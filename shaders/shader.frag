#version 450
#extension GL_EXT_multiview : enable
struct Material {
	vec4 ambientColor;   // Ka: �����ⷴ��ϵ��
	vec4 diffuseColor;   // Kd: ������ϵ��
	vec4 specularColor;  // Ks: ���淴��ϵ��
	float shininess;          // Ns: ����߹�ָ��
	float opacity;
};
struct Light {
	vec4 lightPosition; // ��Դλ��
	vec4 lightColor;    // ��Դ��ɫ
	float lightIntensity;    // ��Դǿ��
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4) flat in int Instanceidx;
layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler;

// �� eye_position ��װ�� uniform block ��
layout(binding = 2) uniform EyePositionBlock {
    vec3 eye_position[2];
};

// �� Material ��װ�� uniform block ��
layout(binding = 3) uniform MaterialBlock {
    Material material;
};

// �� Light ��װ�� uniform block ��
layout(binding = 4) uniform LightBlock {
    Light light;
};
void main() {
   // ����������һ��
    vec3 N = normalize(fragNormal);

    // ��Դ��������
    vec3 L = normalize(vec3(light.lightPosition) - fragPosition);

    // ���߷�������
    vec3 V = normalize(eye_position[gl_ViewIndex] - fragPosition);

    // �������
    vec3 H = normalize(L + V);

    // ����˥��
    float r = length(vec3(light.lightPosition) - fragPosition);
    float attenuation = light.lightIntensity / (r * r);

    // ���������
    vec4 ambient = material.ambientColor * light.lightColor * attenuation;

    // ���������
    float NdotL = max(dot(N, L), 0.0);
    vec4 diffuse = material.diffuseColor * light.lightColor * attenuation * NdotL;

    // ���淴�������Blinn-Phong ʹ�ð��������
    float NdotH = max(dot(N, H), 0.0);
    vec4 specular = material.specularColor * light.lightColor * attenuation * pow(NdotH, material.shininess);

    // �������
    vec4 texColor = texture(texSampler, fragTexCoord);

    // ������ɫ����
    vec4 color = ambient + diffuse + specular;
    color *= texColor;

    outColor = vec4(color.rgb, material.opacity);
    
}