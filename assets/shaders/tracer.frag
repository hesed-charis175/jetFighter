#version 460 core
in  float vAlpha;
out vec4  FragColor;

void main() {
    vec3 core  = vec3(1.0, 1.0, 1.0);
    vec3 glow  = vec3(1.0, 0.05, 0.05);
    vec3 col   = mix(glow, core, vAlpha * vAlpha) * 4.0;
    FragColor  = vec4(col, vAlpha);
}
