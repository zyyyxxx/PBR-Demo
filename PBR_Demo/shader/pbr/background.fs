#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;
uniform samplerCube DebugShadowCubemap;
uniform bool Debug;

float near_plane = 1.0f;
float far_plane = 25.0f;
// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

void main()
{		
    if(!Debug){

        vec3 envColor = texture(environmentMap, WorldPos).rgb;
    
        // HDR tonemap and gamma correct
        envColor = envColor / (envColor + vec3(1.0));
        envColor = pow(envColor, vec3(1.0/2.2)); 
    
        FragColor = vec4(envColor, 1.0);

    }else{

        float depthValue = texture(DebugShadowCubemap, WorldPos).r;
        FragColor = vec4(vec3(LinearizeDepth(depthValue)/far_plane), 1.0); // perspective
    }
    
    
}