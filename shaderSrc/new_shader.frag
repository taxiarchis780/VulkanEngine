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


void main()
{
	vec4 color = texture(texSampler, fragTexCoord);

	outColor = color;
}