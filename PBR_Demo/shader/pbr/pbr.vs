#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// 传递贴图uv坐标 ， 顶点世界坐标，法线世界方向
out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

// mvp变换与法线矩阵
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    TexCoords = aTexCoords;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    mat3 normal = transpose(inverse(mat3(model)));
    Normal = normalMatrix * aNormal;   
    
    gl_Position =  projection * view * vec4(WorldPos, 1.0);
}