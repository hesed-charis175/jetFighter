#version 460 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;
in vec4 vLightSpacePos;
in vec3 vVertColor;

uniform int uUseVertColor;
uniform vec3          uSunDir;
uniform vec3          uBaseColor;
uniform sampler2D     uDiffuse;
uniform int           uHasTex;
uniform sampler2DShadow uShadowMap;
uniform float         uShadowStrength;

float shadowPCF(vec4 lsPos) {
    vec3 proj = lsPos.xyz / lsPos.w;
    proj = proj * 0.5 + 0.5;
    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0 ||
        proj.z > 1.0) return 1.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec3 sc = vec3(proj.xy + vec2(x, y) * texelSize, proj.z - 0.002);
            shadow += texture(uShadowMap, sc);
        }
    }
    return shadow / 9.0;
}
out vec4 FragColor;

void main() {
    vec3 N = normalize(gl_FrontFacing ? vNormal : -vNormal);
    vec3 L = normalize(uSunDir);

vec3 albedo;
if (bool(uUseVertColor)) {
    albedo = vVertColor;
} else if (bool(uHasTex)) {
    albedo = texture(uDiffuse, vUV).rgb;
} else {
    albedo = uBaseColor;
}    float diff    = max(dot(N, L), 0.0);
    float ambient = 0.30;

    vec3 V    = normalize(-vWorldPos);
    vec3 H    = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.4;

    float lit    = mix(1.0, shadowPCF(vLightSpacePos), uShadowStrength);
    vec3  col    = albedo * (ambient + diff * lit) + vec3(spec * lit);
    FragColor    = vec4(col, 1.0);
}
