#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location=3) in vec3 aVertColor;
out vec3 vVertColor;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uLightSpaceMat;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;
out vec4 vLightSpacePos;

void main() {
 vVertColor = aVertColor;
	vec4 world     = uModel * vec4(aPos, 1.0);
  vWorldPos      = world.xyz;
  vNormal        = normalize(mat3(transpose(inverse(uModel))) * aNormal);
  vUV            = aUV;
  vLightSpacePos = uLightSpaceMat * world;
  gl_Position    = uMVP * vec4(aPos, 1.0);
}
