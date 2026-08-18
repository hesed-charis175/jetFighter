#version 460 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
uniform sampler2D uTerrainHeightMap;
uniform float uWorldSize;

out vec3  vWorldPos;
out vec3  vNormal;
out float vHeight;

float heightAt(vec2 xz) {
    vec2 uv = xz / uWorldSize + 0.5;
    return texture(uTerrainHeightMap, uv).r;
}

void main() {
    vec3 p = aPos;
    p.y = heightAt(p.xz);

    vec2 texel = 1.0 / vec2(textureSize(uTerrainHeightMap, 0));
    float e = uWorldSize * texel.x; // one texel, in world units
    float hL = heightAt(p.xz + vec2(-e, 0));
    float hR = heightAt(p.xz + vec2( e, 0));
    float hD = heightAt(p.xz + vec2(0, -e));
    float hU = heightAt(p.xz + vec2(0,  e));
    vNormal  = normalize(vec3(hL-hR, 2.0*e, hD-hU));

    vWorldPos = p;
    vHeight   = p.y;
    gl_Position = uMVP * vec4(p, 1.0);
}
