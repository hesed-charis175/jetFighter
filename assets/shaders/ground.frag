#version 460 core
in  vec3 vPos;
out vec4 FragColor;

void main() {
    vec2 grid = abs(fract(vPos.xz * 0.1 - 0.5) - 0.5) / fwidth(vPos.xz * 0.1);
    float line = min(min(grid.x, grid.y), 1.0);
    vec3 baseColor = vec3(0.12, 0.28, 0.10);
    vec3 lineColor = vec3(0.25, 0.50, 0.20);
    FragColor = vec4(mix(lineColor, baseColor, line), 1.0);
}
