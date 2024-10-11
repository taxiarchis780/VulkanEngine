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
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 aNormal;
layout(location = 4) out vec3 aTangent;
layout(location = 5) out vec3 aBitangent;
layout(location = 6) out vec3 aCameraPos;
layout(location = 7) out vec4 fragLightSpacePos;
layout(location = 8) out Material mMaterial;

void main() {

    // Calculate the vertex position in world space
    vec4 worldPosition = ubo.transform * vec4(inPosition, 1.0);
    fragPos = worldPosition.xyz;

    // Pass texture coordinates to the fragment shader
    fragTexCoord = inTexCoord;

    // Transform the normal, tangent, and bitangent to world space
    mat3 normalMatrix = transpose(inverse(mat3(ubo.transform)));
    aNormal = normalize(vec3(ubo.transform * vec4(inNormal, 0.0)));
    aTangent = normalize(vec3(ubo.transform * vec4(inTangent, 0.0)));

    aTangent = normalize(aTangent - dot(aTangent, aNormal) * aNormal);

    aBitangent = cross(aNormal, aTangent);
    
    // Pass lighting information to the fragment shader
    aCameraPos = ubo.cameraPos;

    mMaterial = ubo.material;

    fragLightSpacePos = ubo.lightSpaceMat * vec4(fragPos, 1.0);    

    // Calculate the final vertex position in clip space
    gl_Position = ubo.proj * ubo.view * worldPosition;
    
    fragColor = inColor;
}