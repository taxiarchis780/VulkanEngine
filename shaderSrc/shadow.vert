#version 460 core
#extension GL_EXT_ray_tracing : disable


struct Material 
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 shininess;
    vec3 overrideColor;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 transform;
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMat;
    vec3 cameraPos;
    Material material;
    float time;
} ubo;

layout(location = 0) in vec3 inPosition;


void main() {

    gl_Position = ubo.lightSpaceMat * ubo.transform * vec4(inPosition, 1.0);
}