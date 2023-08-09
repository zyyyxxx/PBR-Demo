#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMetallic_Roughness_AO;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
    float Radius;
};

uniform int NR_LIGHTS;
uniform Light lights[4];
uniform vec3 viewPos;

const float PI = 3.14159265359;

// shadow
// -----------------------------------------------------
uniform samplerCube depthCubemaps[4];
uniform float far_plane;
uniform bool shadows;

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculation(vec3 fragPos , int lightIndex)
{
    float bias = 0.15;
    int samples = 20;
    float shadow = 0.0;

    // 光线指向片段
    vec3 fragToLight = fragPos - lights[lightIndex].Position;

    // 光线到片段的深度
    float currentDepth = length(fragToLight);
    
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0; 
    //我们可以基于观察者里一个fragment的距离来改变diskRadius；
    //这样我们就能根据观察者的距离来增加偏移半径了，当距离更远的时候阴影更柔和，更近了就更锐利。
    
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthCubemaps[lightIndex], fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= far_plane;   // 从 [0;1] 映射回采样时的原始距离
        
        if(currentDepth - bias > closestDepth){
            shadow += 1.0;
        }
    }
    shadow /= float(samples);
    
    // 1为完全阴影
    return shadow;
}
// -----------------------------------------------------



float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   




void main()
{             
    // 从gbuffer中获取信息
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 WorldPos = FragPos;
    vec3 camPos = viewPos;

    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 N = Normal;

    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
    float metallic = texture(gMetallic_Roughness_AO, TexCoords).r;
    float roughness = texture(gMetallic_Roughness_AO, TexCoords).g;
    float ao = texture(gMetallic_Roughness_AO, TexCoords).b;
    
    // 向量信息
    vec3 V = normalize(camPos - WorldPos);// 片段位置指向相机 观察方向
    vec3 R = reflect(-V, N); 

    // 如果是非导电材质（如塑料），则使用 F0 为 0.04；
    // 如果是金属，则使用反照率颜色作为 F0（使用金属工作流程）。
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 viewDir  = V;
    vec3 Lo = vec3(0.0);    

    // 遍历光源进行直接光照着色
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // 阴影映射
        float shadow = ShadowCalculation(FragPos , i);

        vec3 L = normalize(lights[i].Position - WorldPos);
        vec3 H = normalize(V + L); // V和L的半程向量

        float distance = length(lights[i].Position - FragPos);
        float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
        //float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lights[i].Color * attenuation;

        if(shadows == false) shadow = 0;
        radiance *= (1-shadow);

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS = Fresnel
        vec3 kS = F;

        // 为了能量守恒，漫反射和高光反射光线的总和不能超过1.0
        // （除非表面本身发射光线）；为了保持这一关系，漫反射成分（kD）应该等于1.0 - 高光反射成分（kS）
        vec3 kD = vec3(1.0) - kS;

        // 将漫反射成分（kD）乘以反金属性的补数，
        // 这样只有非金属材质才会有漫反射光照，而对于部分金属材质会进行线性混合（纯金属材质没有漫反射光照）。
        kD *= 1.0 - metallic;	           

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);    
        
        // 添加到出射的radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
        // 请注意，我们已经通过菲涅尔（kS）将BRDF乘以了一次，因此我们不会再次将其乘以kS 

    }    

    // IBL
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;

    // 采样预过滤贴图和 BRDF LUT，并按照 Split-Sum 近似方法将它们组合在一起，以获得基于间接光照的镜面反射部分。    
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 color = ambient + Lo;

    // HDR 色调映射
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color , 1.0);
    
}