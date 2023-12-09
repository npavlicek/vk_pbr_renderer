#version 450

layout(location = 0) in vec2 texCoord;

layout(binding = 0) uniform samplerCube cubeMap;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(texture(cubeMap, texCoord), 1.0);
}
