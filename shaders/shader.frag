#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoords;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 4) uniform CameraPos {
	vec3 pos;
} cameraPos;

layout(set = 1, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 1, binding = 1) uniform sampler2D metallicSampler;
layout(set = 1, binding = 2) uniform sampler2D roughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D normalSampler;

vec3 lightPos = vec3(-5.0, -4.0, -5.0);

float distGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
	denom  = 3.1415 * denom * denom;
	return a2 / denom;
}

float GeometryGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx1 = GeometryGGX(NdotV, k);
	float ggx2 = GeometryGGX(NdotL, k);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
	vec3 albedo = texture(diffuseSampler, fragTexCoords).rgb;
	float metallic = texture(metallicSampler, fragTexCoords).r;
	float roughness = texture(roughnessSampler, fragTexCoords).r;
	vec3 Normal = texture(normalSampler, fragTexCoords).rgb;
	Normal = normalize(Normal * 2.0 - 1.0);
	Normal = normalize(TBN * Normal);
	float ao = 1.0;

	// Camera position in local space
	vec3 localCamPos = normalize(cameraPos.pos - worldPos);

	vec3 F0 = vec3(0.04);
	F0  = mix(F0, albedo, metallic);

	vec3 localLight = normalize(lightPos - worldPos);
	vec3 halfWayVector = normalize(localCamPos + localLight);
	float distance = length(lightPos - worldPos);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = vec3(50.f, 50.f, 50.f) * attenuation;

	// Cook torrance equation
	float NDF = distGGX(Normal, halfWayVector, roughness);
	float G = GeometrySmith(Normal, localCamPos, localLight, roughness);
	vec3 F = fresnelSchlick(max(dot(halfWayVector, localCamPos), 0.0), F0);

	// Specular
	vec3 kS = F;

	// Diffuse
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(Normal, localCamPos), 0.0) * max(dot(Normal, localLight), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;

	float NdotL = max(dot(Normal, lightPos), 0.0);
	vec3 Lo = (kD * albedo / 3.1415 + specular) * radiance * NdotL;

	vec3 ambient = vec3(0.03) * albedo * ao;
	vec3 color = ambient + Lo;

	outColor = vec4(color, 1.0);
}
