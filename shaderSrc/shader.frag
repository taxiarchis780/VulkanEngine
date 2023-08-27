#version 460 core
#extension GL_EXT_ray_tracing : enable


layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 lightPos;
layout(location = 5) in vec3 aCameraPos;
layout(location = 6) in vec3 alightColor;
layout(location = 7) in vec3 mambient;
layout(location = 8) in vec3 mdiffuse;
layout(location = 9) in vec3 mspecular;
layout(location = 10) in float mshininess;


layout(location = 0) out vec4 outColor;

void main() {

    // ambient
    float ambientStrength = 0.05f;
    vec3 ambient = mambient * ambientStrength * alightColor;
    
    // diffuse
    vec3 norm = normalize(aNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0f);
    
    vec3 diffuse = (diff * mdiffuse )* alightColor;
    
    // specular
    float specularStrength = 0.1f;
    vec3 viewDir = normalize(aCameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.1f), mshininess);
    vec3 specular = specularStrength * (spec * alightColor * mspecular);


    vec3 result = (ambient + diffuse + specular) * fragColor;
    float gamma = 2.2f;
    result = pow(result, vec3(1.0f/gamma));
    outColor = texture(texSampler, fragTexCoord) * vec4(result, 1.0f);
    
    //outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}