#version 460 core
in  vec3  vWorldPos;
in  vec3  vNormal;
in  float vHeight;
out vec4  FragColor;

uniform vec3  uSunDir;
uniform float uSunElev;
uniform sampler2DShadow uShadowMap;
uniform mat4  uShadowMatrix;
uniform float uPlaneAlt;

float hash(vec2 p){p=fract(p*vec2(127.1,311.7));p+=dot(p,p+19.19);return fract(p.x*p.y);}
float noise(vec2 p){
    vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),
               mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
}
float fbm(vec2 p){
    float v=0.,a=.5;
    for(int i=0;i<5;i++){v+=a*noise(p);p*=2.03;a*=.49;}
    return v;
}

float triNoise(vec3 pos, float scale, vec3 n) {
    vec3 blend = abs(n);
    blend = pow(blend, vec3(4.0));
    blend /= dot(blend, vec3(1.0));
    float xz = noise(pos.xz * scale);
    float xy = noise(pos.xy * scale);
    float yz = noise(pos.yz * scale);
    return xz*blend.y + xy*blend.z + yz*blend.x;
}

void main() {
    float h     = vHeight;
    float slope = 1.0 - clamp(dot(vNormal, vec3(0,1,0)), 0.0, 1.0);

    vec3 snowCol   = vec3(0.93,0.94,0.97);
    vec3 iceCol    = vec3(0.78,0.86,0.95);
    vec3 rockCol   = vec3(0.42,0.36,0.29);
    vec3 rockDark  = vec3(0.28,0.24,0.20);
    vec3 alpineCol = vec3(0.28,0.36,0.20);
    vec3 grassCol  = vec3(0.20,0.40,0.13);
    vec3 dryGrass  = vec3(0.45,0.42,0.22);
    vec3 dirtCol   = vec3(0.40,0.31,0.20);
    vec3 sandCol   = vec3(0.70,0.62,0.42);
    vec3 gravelCol = vec3(0.50,0.46,0.38);

    float n1 = noise(vWorldPos.xz * 0.03);
    float n2 = noise(vWorldPos.xz * 0.008);

    vec3 color;
    float snowLine = mix(680.0, 820.0, n2);
    float rockLine = mix(480.0, 560.0, n1);

    if (h > snowLine + 120.0)
        color = mix(snowCol, iceCol, n1);
    else if (h > snowLine)
        color = mix(rockCol, snowCol, smoothstep(snowLine, snowLine+120.0, h));
    else if (h > rockLine)
        color = mix(alpineCol, rockCol, smoothstep(rockLine, rockLine+100.0, h));
    else if (h > 250.0)
        color = mix(grassCol, alpineCol, smoothstep(250.0, rockLine, h));
    else if (h > 90.0)
        color = mix(dirtCol, mix(grassCol,dryGrass,n1), smoothstep(90.0,200.0,h));
    else if (h > 30.0)
        color = mix(sandCol, dirtCol, smoothstep(30.0, 90.0, h));
    else
        color = mix(gravelCol, sandCol, n1);

    float slopeMask = smoothstep(0.38, 0.72, slope);
    color = mix(color, mix(rockCol, rockDark, n2), slopeMask);

    float coarse = triNoise(vWorldPos * 0.001, 1.0, vNormal);
    color *= 0.82 + 0.36 * coarse;

    float mid = triNoise(vWorldPos * 0.004, 1.0, vNormal);
    color *= 0.88 + 0.24 * mid;

    float fine = triNoise(vWorldPos * 0.018, 1.0, vNormal);
    color *= 0.91 + 0.18 * fine;

    float strata = smoothstep(0.48, 0.52,
        noise(vec2(vWorldPos.y * 0.012, vWorldPos.x * 0.003)));
    float strataMask = slopeMask * (1.0 - smoothstep(400.0, 700.0, h));
    color = mix(color, color * 0.72, strata * strataMask * 0.6);

    vec4 sc = uShadowMatrix * vec4(vWorldPos, 1.0);
    float shadowSample = textureProj(uShadowMap, sc);
    float altFade  = 1.0 - smoothstep(0.0, 1500.0, uPlaneAlt);
    vec2  shadowNDC = sc.xy / sc.w;
    float edgeFade = 1.0 - smoothstep(0.6, 1.0, max(abs(shadowNDC.x), abs(shadowNDC.y)));
    float shadow   = mix(1.0, shadowSample, altFade * edgeFade);
    shadow = mix(1.0, shadow, smoothstep(-0.05, 0.15, uSunElev));

    float diff    = max(dot(vNormal, uSunDir), 0.0);
    float ambient = mix(0.06, 0.22, smoothstep(-0.1, 0.3, uSunElev));
    float ao      = clamp(dot(vNormal, vec3(0,1,0))*0.5+0.5, 0.0, 1.0);
    float light   = ambient * ao + diff * 0.78 * shadow;

    vec3 sunTint = mix(vec3(1.0,0.55,0.25), vec3(1.0,1.0,1.0),
                       smoothstep(-0.05, 0.25, uSunElev));
    color *= light * sunTint;

    float dist = length(vWorldPos.xz);
    float fog  = smoothstep(5000.0, 10000.0, dist);
    vec3 fogCol = mix(vec3(0.65,0.75,0.88), vec3(0.55,0.38,0.25),
                      smoothstep(0.2,-0.05,uSunElev));
    color = mix(color, fogCol, fog);

    FragColor = vec4(color, 1.0);
}
