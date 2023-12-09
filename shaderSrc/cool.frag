#version 460 core
#extension GL_EXT_ray_tracing : disable

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
layout(location = 10) in vec3 mshininess;


layout(location = 0) out vec4 outColor;


float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * 0.1f * 100.0f) / (100.0f + 0.1f - z * (100.0f - 0.1f));	
}


void main() {
    // Sample texture
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Calculate the depth difference between neighboring fragments
    float depth = LinearizeDepth(gl_FragCoord.z) / 100.0f;
    float depthNeighbour = texture(texSampler, fragTexCoord + vec2(0.001, 0.00)).r;
    float depthDiff = depth - depthNeighbour;

    float outlineThreshold = 1.0f;

    vec3 outlineColor = vec3(1.0f, 1.0f, 1.0f);

    vec3 finalColor = mix(texColor.rgb, outlineColor, smoothstep(0.0f, outlineThreshold, abs(depthDiff)));
    outColor = vec4(finalColor, 1.0f);
}