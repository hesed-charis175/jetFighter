#version 460 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;

out vec3  vWorldPos;
out vec3  vNormal;
out float vHeight;

float hash(vec2 p) {
    p = fract(p * vec2(127.1,311.7));
    p += dot(p, p+19.19);
    return fract(p.x*p.y);
}
float noise(vec2 p) {
    vec2 i=floor(p), f=fract(p);
    f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),
               mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
}
float ridged(vec2 p) { return 1.0 - abs(noise(p)*2.0-1.0); }

float fbm(vec2 p) {
    float v=0.0,a=0.5;
    for(int i=0;i<8;i++){v+=a*noise(p);p*=2.03;a*=0.49;}
    return v;
}
float fbmRidged(vec2 p) {
    float v=0.0,a=0.5;
    for(int i=0;i<7;i++){v+=a*ridged(p);p*=2.07;a*=0.48;}
    return v;
}

float heightAt(vec2 xz) {
    vec2 p = xz * 0.00035;
    float continent = fbm(p * 0.18) * 0.6 + 0.4;
    float ridge     = fbmRidged(p * 0.9) * continent;
    float hills     = fbm(p * 1.8  + vec2(4.2, 1.7)) * 0.3;
float detail    = fbm(p * 6.0  + vec2(2.1, 3.3)) * 0.07;
float h = mix(hills, ridge, smoothstep(0.2, 0.7, ridge));
h = h * continent + detail;
	h = pow(h, 1.35);  
return max(h * 1100.0, -5.0);
}

void main() {
    vec3 p = aPos;
    p.y = heightAt(p.xz);

    float e  = 1.2;
    float hL = heightAt(p.xz+vec2(-e, 0));
    float hR = heightAt(p.xz+vec2( e, 0));
    float hD = heightAt(p.xz+vec2(0, -e));
    float hU = heightAt(p.xz+vec2(0,  e));
    vNormal  = normalize(vec3(hL-hR, 2.0*e, hD-hU));

    vWorldPos = p;
    vHeight   = p.y;
    gl_Position = uMVP * vec4(p, 1.0);
}
