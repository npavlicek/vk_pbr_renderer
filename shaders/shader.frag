#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 4) uniform CameraPos {
	vec3 pos;
} cameraPos;

layout(set = 1, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 1, binding = 1) uniform sampler2D metallicSampler;
layout(set = 1, binding = 2) uniform sampler2D roughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D normalSampler;

void main() {
    //outColor = vec4(fragTexCoords, 0.0, 1.0);
	vec4 preColor = texture(diffuseSampler, fragTexCoords);
    outColor = vec4(preColor.x * cameraPos.pos.x, preColor.yzw);
	//outColor = vec4(fragColor, 1.0);
}
