#version 460 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;

uniform mat4      uMVP;
uniform mat4      uModel;
uniform sampler2D uHeightmap;
uniform float     uTime;

out vec3 vWorldPos;
out vec2 vUV;
out vec3 vNormal;
out float vFoam;

void main() {
    float h  = texture(uHeightmap, aUV).r;

    // finite diff normal from heightmap
    vec2 texel = 1.0 / vec2(textureSize(uHeightmap, 0));
    float hL = texture(uHeightmap, aUV + vec2(-texel.x, 0)).r;
    float hR = texture(uHeightmap, aUV + vec2( texel.x, 0)).r;
    float hD = texture(uHeightmap, aUV + vec2(0, -texel.y)).r;
    float hU = texture(uHeightmap, aUV + vec2(0,  texel.y)).r;
    vNormal  = normalize(vec3(hL - hR, 0.015, hD - hU));

    vec3 p   = aPos;
    p.y      = h;

    vWorldPos = vec3(uModel * vec4(p, 1.0));
    vUV       = aUV;
    vFoam     = smoothstep(0.8, 2.2, h);
    gl_Position = uMVP * vec4(p, 1.0);
}
