#version 330 core
// 绑定渲染到哪个颜色附件
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gMetallic_Roughness_AO;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    vec3 WorldPos = FragPos;
    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{    

    vec3 albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    float metallic = texture(metallicMap, TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).r;
    float ao = texture(aoMap, TexCoords).r;

    // 输出片段位置
    gPosition = FragPos;

    // 输出法线
    //gNormal = normalize(Normal);
    gNormal = getNormalFromMap();

    // 输出Albedo
    gAlbedo.rgb = albedo;

    // 输出Metallic_Roughness_AO
    gMetallic_Roughness_AO.r = metallic;
    gMetallic_Roughness_AO.g = roughness;
    gMetallic_Roughness_AO.b = ao;

}