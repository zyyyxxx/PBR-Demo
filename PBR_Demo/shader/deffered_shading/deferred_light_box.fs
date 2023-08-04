#version 330 core
layout (location = 0) out vec4 FragColor;

uniform vec3 lightColor;

void main()
{           
    // HDR 色调映射
    vec3 Color = lightColor;
    Color = Color / (Color + vec3(1.0));
    // gamma correct
    Color = pow(Color, vec3(1.0/2.2)); 
    FragColor = vec4(Color, 1.0);
}