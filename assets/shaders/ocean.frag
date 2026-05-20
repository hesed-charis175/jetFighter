#version 460 core
in  vec3  vWorldPos;
in  vec2  vUV;
in  vec3  vNormal;
in  float vFoam;
out vec4  FragColor;

uniform vec3      uSunDir;
uniform float     uSunElev;
uniform float     uTime;
uniform sampler2D uHeightmap;

float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

vec3 noiseGrad(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u  = f * f * (3.0 - 2.0 * f);
    vec2 du = 6.0 * f * (1.0 - f);

    float a = hash(i);
    float b = hash(i + vec2(1,0));
    float c = hash(i + vec2(0,1));
    float d = hash(i + vec2(1,1));

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = a - b - c + d;

    return vec3(
        k0 + k1*u.x + k2*u.y + k3*u.x*u.y,             
        du.x * (k1 + k3*u.y),                         
        du.y * (k2 + k3*u.x)                         
    );
}

vec3 fbmHiG(vec2 p) {
    vec3 r = vec3(0.0); float a = 0.5; float scale = 1.0;
    for (int i = 0; i < 6; i++) {
        vec3 n = noiseGrad(p);
        r.x  += a * n.x;
        r.yz += a * n.yz * scale; 
        p    *= 2.13; a *= 0.47; scale *= 2.13;
    }
    return r;
}

vec3 fbmMidG(vec2 p) {
    vec3 r = vec3(0.0); float a = 0.5; float scale = 1.0;
    for (int i = 0; i < 5; i++) {
        vec3 n = noiseGrad(p);
        r.x  += a * n.x;
        r.yz += a * n.yz * scale;
        p    *= 2.07; a *= 0.48; scale *= 2.07;
    }
    return r;
}

vec3 fbmLoG(vec2 p) {
    vec3 r = vec3(0.0); float a = 0.5; float scale = 1.0;
    for (int i = 0; i < 3; i++) {
        vec3 n = noiseGrad(p);
        r.x  += a * n.x;
        r.yz += a * n.yz * scale;
        p    *= 2.1;  a *= 0.5;  scale *= 2.1;
    }
    return r;
}

void main() {
    vec3 N = normalize(vNormal);

    vec2 r1 = vUV * 9.0  + vec2( uTime*0.020,  uTime*0.013);
    vec2 r2 = vUV * 23.0 + vec2(-uTime*0.028,  uTime*0.021);
    vec2 r3 = vec2(vUV.x+vUV.y, vUV.x-vUV.y) * 14.0 + vec2(uTime*0.015, -uTime*0.017);
    vec2 s1 = vWorldPos.xz * 0.0008 + vec2( uTime*0.010, -uTime*0.007);
    vec2 s2 = vWorldPos.xz * 0.00018 + vec2(-uTime*0.005,  uTime*0.003);

    vec3 g1 = fbmHiG(r1); 
    vec3 g2 = fbmHiG(r2);
    vec3 g3 = fbmHiG(r3);
    vec3 g4 = fbmMidG(s1);
    vec3 g5 = fbmLoG(s2);

    vec2 totalFD = g1.yz * 1.0
                 + g2.yz * 0.6
                 + g3.yz * 0.5
                 + g4.yz * 0.7
                 + g5.yz * 0.9;

    vec3 detailN = normalize(vec3(totalFD.x, 0.10, totalFD.y));
    N = normalize(N + detailN * 0.55);

    vec3 deep    = vec3(0.03, 0.22, 0.58);
    vec3 shallow = vec3(0.08, 0.52, 0.78);
    vec3 water   = mix(deep, shallow, smoothstep(-2.0, 3.0, vWorldPos.y));

    float sss = pow(clamp(vWorldPos.y / 3.0, 0.0, 1.0), 1.5);
    water = mix(water, vec3(0.06, 0.68, 0.56), sss * 0.55);

    float cosA    = clamp(dot(N, vec3(0,1,0)), 0.0, 1.0);
    float fresnel = mix(0.04, 0.95, pow(1.0 - cosA, 4.0));
    vec3  dayRefl   = vec3(0.42, 0.65, 0.90);
    vec3  duskRefl  = vec3(0.80, 0.45, 0.18);
    vec3  nightRefl = vec3(0.04, 0.06, 0.14);
    float elevDay  = smoothstep(-0.05, 0.28, uSunElev);
    float elevNight= smoothstep(-0.10, 0.10, uSunElev);
    vec3  skyRefl  = mix(nightRefl, mix(duskRefl, dayRefl, elevDay), elevNight);
    water = mix(water, skyRefl, fresnel * 0.55);

    float ambient = mix(0.40, 0.70, smoothstep(-0.1, 0.3, uSunElev));
    float diff    = max(dot(N, uSunDir), 0.0) * 0.65;
    water *= (ambient + diff);

    vec3  H   = normalize(uSunDir + vec3(0,1,0));
    float NdH = max(dot(N, H), 0.0);
    float spec = pow(NdH, 512.0)
               + pow(NdH,  28.0) * 0.18
               + pow(NdH,   6.0) * 0.04;
    vec3  sunC = mix(vec3(1.0,0.48,0.10), vec3(1.0,0.97,0.88),
                     smoothstep(-0.1, 0.28, uSunElev));
    water += sunC * spec * smoothstep(-0.04, 0.06, uSunElev);

    water += vec3(0.04, 0.06, 0.12) * smoothstep(0.05, -0.08, uSunElev);

    float foamA = g1.x;  
    float foamB = g4.x; 
    float foam  = smoothstep(0.15, 0.55, vFoam + foamA * 0.30 + foamB * 0.15);
    water = mix(water, vec3(0.90, 0.94, 0.97), foam);

    float dist = length(vWorldPos.xz);
    float fog  = smoothstep(4000.0, 9000.0, dist);
    vec3  fogC = mix(vec3(0.58, 0.72, 0.88), vec3(0.52, 0.38, 0.24),
                     smoothstep(0.2, -0.05, uSunElev));
    water = mix(water, fogC, fog);

    FragColor = vec4(water, 1.0);
}
