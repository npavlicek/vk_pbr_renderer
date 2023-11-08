#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D diffuseSampler;
layout(binding = 1) uniform sampler2D metallicSampler;
layout(binding = 2) uniform sampler2D roughnessSampler;
layout(binding = 3) uniform sampler2D normalSampler;


void main() {
    //outColor = vec4(fragTexCoords, 0.0, 1.0);
    outColor = texture(diffuseSampler, fragTexCoords);
	//outColor = vec4(fragColor, 1.0);
}
