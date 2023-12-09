#version 460 core
#extension GL_EXT_ray_tracing : disable

struct Material 
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 shininess;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 transform;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    vec3 lightPos;
    vec3 lightColor;
    Material material;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 aNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec3 lightPos;
layout(location = 5) out vec3 aCameraPos;
layout(location = 6) out vec3 alightColor;
layout(location = 7) out Material mMaterial;


float random (in vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))
                * 43758.5453123);
}

void main() {
    
    vec3 aInPosition = inPosition;
    aInPosition.y += random(inTexCoord); 

    gl_Position = ubo.proj * ubo.view * ubo.transform * vec4(aInPosition, 1.0);
    fragColor = inColor;
    aNormal = mat3(transpose(inverse(ubo.transform))) * normal;  
    lightPos = ubo.lightPos;//vec3(2.3f, -2.6f, 5.0f);
    alightColor = ubo.lightColor;
    fragPos = vec3(ubo.transform * vec4(inPosition, 1.0));
    aCameraPos = ubo.cameraPos;
    fragTexCoord = inTexCoord;
    
    mMaterial = ubo.material;
    

}