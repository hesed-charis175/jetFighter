#version 460 core
in  vec3 vDir;
out vec4 FragColor;

uniform vec3  uSunDir;
uniform float uTime;

float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}
float hash3(vec3 p) {
    p = fract(p * vec3(127.1, 311.7, 74.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y + p.y * p.z);
}
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f*f*(3.0-2.0*f);
    return mix(
        mix(hash(i),           hash(i+vec2(1,0)), f.x),
        mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), f.x), f.y);
}
float fbm(vec2 p) {
    float v=0.0, a=0.5;
    for(int i=0;i<6;i++){ v+=a*noise(p); p*=2.1; a*=0.5; }
    return v;
}

float stars(vec3 dir, float threshold) {
    vec3 d    = normalize(dir);
    float scale = 400.0;
    vec3 cell = floor(d * scale);
    float s   = hash3(cell);
    if (s < threshold) return 0.0;
    vec3 jitter = vec3(hash3(cell+1.3), hash3(cell+2.7), hash3(cell+4.1)) - 0.5;
    float dist  = length(fract(d * scale) - 0.5 + jitter * 0.25);
    float size  = mix(0.18, 0.08, (s - threshold) / (1.0 - threshold));
    return smoothstep(size, 0.0, dist);
}

float starBrightness(vec3 dir) {
    float s1 = stars(dir, 0.980) * 0.4; 
    float s2 = stars(dir * 1.31, 0.991) * 0.75;
    float s3 = stars(dir * 0.73, 0.996) * 1.0;
    return s1 + s2 + s3;
}

void skyPalette(float elev,
    out vec3 zenith, out vec3 horizon, out vec3 haze) {

    vec3 zDay  = vec3(0.08, 0.20, 0.62);
    vec3 hDay  = vec3(0.40, 0.66, 0.94);
    vec3 haDay = vec3(0.78, 0.88, 0.96);

    vec3 zNight  = vec3(0.003, 0.005, 0.025);
    vec3 hNight  = vec3(0.008, 0.012, 0.045);
    vec3 haNight = vec3(0.015, 0.018, 0.055);

    vec3 zDusk  = vec3(0.04, 0.04, 0.18);
    vec3 hDusk  = vec3(0.75, 0.28, 0.08);
    vec3 haDusk = vec3(1.00, 0.52, 0.12);

    float day  = smoothstep(0.0,  0.20, elev);
    float dusk = smoothstep(-0.28,-0.04, elev) *
                 smoothstep( 0.32, 0.08, elev);

    zenith  = mix(mix(zNight,  zDusk,  dusk), zDay,  day);
    horizon = mix(mix(hNight,  hDusk,  dusk), hDay,  day);
    haze    = mix(mix(haNight, haDusk, dusk), haDay, day);
}

void main() {
    vec3 dir     = normalize(vDir);
    float sunElev = uSunDir.y;

    vec3 zenith, horizon, haze;
    skyPalette(sunElev, zenith, horizon, haze);

    float up   = clamp( dir.y, 0.0, 1.0);
    float down = clamp(-dir.y, 0.0, 1.0);
    float h    = 1.0 - up;

    vec3 sky = mix(mix(zenith, horizon, pow(h, 2.2)), haze, pow(h, 7.0));

    float horizonBand = exp(-abs(dir.y) * 8.0);
    vec3  horizonGlow = mix(vec3(0.01,0.02,0.08), haze, smoothstep(-0.1,0.3,sunElev));
    sky = mix(sky, horizonGlow * 1.3, horizonBand * 0.35);

    float sunDot  = dot(dir, uSunDir);
    float sunVis  = smoothstep(-0.06, 0.06, sunElev);
    vec3  sColor  = mix(vec3(1.0,0.4,0.08), vec3(1.0,0.97,0.88),
                        smoothstep(-0.1, 0.25, sunElev));
    float disc    = smoothstep(0.9995, 0.99975, sunDot);
    float glow    = pow(clamp(sunDot,0.0,1.0), 56.0) * 0.65;
    float halo    = pow(clamp(sunDot,0.0,1.0),  5.0) * 0.14;
    float scatter = pow(clamp(sunDot,0.0,1.0),  2.5)
                  * smoothstep(0.18,-0.08, sunElev)
                  * smoothstep(-0.12,0.0,  sunElev);
    sky += sColor * (disc*3.5 + glow + halo) * sunVis;
    sky += vec3(1.0,0.32,0.04) * scatter * 1.4;

    vec3 moonDir = normalize(vec3(-uSunDir.x, -uSunDir.y, -uSunDir.z));
    moonDir = normalize(moonDir + vec3(0, 0.05, 0));
    float moonVis  = smoothstep(0.04,-0.04, sunElev);
    float moonDot  = dot(dir, moonDir);
    float moonDisc = smoothstep(0.9990, 0.9995, moonDot);
    float moonGlow = pow(clamp(moonDot,0.0,1.0), 24.0) * 0.12;
    float moonHorizon = exp(-abs(dir.y)*6.0) * moonVis * 0.08;
    sky += vec3(0.82,0.88,1.00) * (moonDisc*2.2 + moonGlow) * moonVis;
    sky += vec3(0.5,0.55,0.7)   * moonHorizon;

    float nightFactor = smoothstep(0.08,-0.08, sunElev);
    if (nightFactor > 0.001) {
        float mw = exp(-pow(dir.x * 0.6 + dir.y * 0.3, 2.0) * 12.0);
        float mwNoise = fbm(dir.xz * 3.0 + vec2(1.7, 0.3));
        sky += vec3(0.10, 0.12, 0.22) * mw * mwNoise * nightFactor * 0.6;
    }

    if (dir.y > -0.15 && nightFactor > 0.001) {
        float fade   = smoothstep(-0.15, 0.05, dir.y);
        float s      = starBrightness(dir);
        float twinkle = 0.82 + 0.18 * sin(uTime * 1.8 + hash3(floor(dir*400.0))*80.0);
        float starHue = hash3(floor(dir*400.0) + 9.9);
        vec3  starCol = mix(vec3(1.0, 0.85, 0.7),
                            mix(vec3(0.85,0.92,1.0), vec3(1.0,1.0,1.0), starHue),
                            step(0.5, starHue));
        sky += starCol * s * twinkle * nightFactor * fade;
    }

    float cloudVis = smoothstep(-0.18, 0.12, sunElev);
    if (dir.y > 0.01 && cloudVis > 0.001) {
        vec2 uv   = dir.xz / max(dir.y, 0.001) * 0.32;
        uv.x     += uTime * 0.00015;
        uv.y     += uTime * 0.00006;

        float c1 = fbm(uv);
        float c2 = fbm(uv * 1.8 + vec2(3.2, 1.7)) * 0.5;
        float cloud = smoothstep(0.46, 0.68, c1 + c2 * 0.4);

        vec3 cLit  = mix(vec3(1.0,0.52,0.22), vec3(1.0,0.98,0.94),
                         smoothstep(0.0, 0.25, sunElev));
        vec3 cShad = mix(vec3(0.28,0.15,0.22), vec3(0.60,0.63,0.72),
                         smoothstep(0.0, 0.25, sunElev));
        float lit  = pow(clamp(dot(dir, uSunDir),0.0,1.0), 2.5);
        vec3  cCol = mix(cShad, cLit, lit);

        float fade = smoothstep(0.01, 0.14, dir.y);
        sky = mix(sky, cCol, cloud * fade * cloudVis * 0.93);
    }

    sky = mix(sky, haze * 0.3, pow(down, 2.5) * 0.5);

    FragColor = vec4(sky, 1.0);
}
