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


layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalMap;



struct Light
{
    vec3 lightPos;
    vec3 lightColor;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

layout(binding = 3) uniform LightUniformBufferObject {
    vec3 pos;
    vec3 rot;
    vec3 color;
} lubo;

layout(binding = 4) uniform sampler2DShadow shadowMap;


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec3 aTangent;
layout(location = 5) in vec3 aBitangent;
layout(location = 6) in vec3 aCameraPos;
layout(location = 7) in vec4 fragLightSpacePos;
layout(location = 8) in Material mMaterial;

layout(location = 0) out vec4 outColor;

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * 0.1f * 100.0f) / (100.0f + 0.1f - z * (100.0f - 0.1f));	
}

float calculateShadow(vec4 fragPosLightSpace) 
{
    
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, fragPosLightSpace.xyz); 
    // get depth of current fragment from light's perspective
    float currentDepth = fragPosLightSpace.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 0.3 : 1.0f;
    
    return shadow;
}


void main() {
    

    vec3 normal = normalize(aNormal);
    vec3 tangent = normalize(aTangent); 
    vec3 bitangent = normalize(aBitangent);
    
    mat3 TBN = mat3(tangent, bitangent, normal);
    vec3 normalMapValue = texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0;
    //vec3 perturbedNormal = normalize(TBN * normalMapValue);

    

    float shadow = calculateShadow(fragLightSpacePos); 
    // ambient
    float ambientStrength = 0.05f;
    vec3 ambient = mMaterial.ambient * ambientStrength * lubo.color;
    // diffuse
    vec3 norm = normalize(aNormal);
    vec3 lightDir = normalize(lubo.pos - fragPos);
    //vec3 lightDir = normalize(lubo.rot);
    float diff = max(dot(norm, lightDir), 0.0f);
    
    vec3 diffuse = (diff * mMaterial.diffuse )* lubo.color;
    
    
    
    // specular
    float specularStrength = mMaterial.shininess.r;
    vec3 viewDir = normalize(aCameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.1f), mMaterial.shininess.r);
    vec3 specular = specularStrength * (spec * lubo.color * mMaterial.specular);


    vec3 result = (ambient + diffuse + specular) * fragColor * mMaterial.overrideColor;
    //vec3 result = (ambient + (shadow) * (diffuse + specular)) * fragColor * mMaterial.overrideColor;    

    float gamma = 2.2f;
    result = pow(result, vec3(1.0f/gamma));

    result *= shadow;
    //float depth = LinearizeDepth(depthValue) / 100.0f;


    outColor =  texture(texSampler, fragTexCoord) * vec4(result, 1.0f);
    //outColor = vec4(texture(shadowMap, projCoords.xyz));
    //outColor = vec4(vec3(depth), 1.0);

}