#version 450

layout (push_constant) uniform UniformBufferOBJ {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoords;
layout(location = 2) out vec3 worldPos;
layout(location = 3) out mat3 TBN;


void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragColor = inColor;
    fragTexCoords = inTexCoord;
	worldPos = (ubo.model * vec4(inPosition, 1.0)).xyz;
	mat3 model = mat3(ubo.model);
	vec3 T = normalize(model * inTangent);
	vec3 B = normalize(model * cross(inNormal, inTangent));
	vec3 N = normalize(model * inNormal);
	TBN = mat3(T, B, N);
}
