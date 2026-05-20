#version 460 core
layout(location=0) in vec3 aOrigin;  
layout(location=1) in vec3 aDir;    
layout(location=2) in float aLife; 

uniform mat4 uVP;
uniform vec3 uCamPos;

out float vAlpha;

void main() {
    float half_len = 12.0;
    float half_w   = 0.20;

    vec3 toCamera = normalize(uCamPos - aOrigin);
    vec3 side = normalize(cross(aDir, toCamera)) * half_w;

    vec3 A = aOrigin - aDir * half_len - side;
    vec3 B = aOrigin - aDir * half_len + side;
    vec3 C = aOrigin + aDir * half_len + side;
    vec3 D = aOrigin + aDir * half_len - side;

    vec3 pos;
    int vid = gl_VertexID % 6;
    if      (vid == 0) pos = A;
    else if (vid == 1) pos = B;
    else if (vid == 2) pos = C;
    else if (vid == 3) pos = A;
    else if (vid == 4) pos = C;
    else               pos = D;

    vAlpha = (1.0 - aLife) * (1.0 - aLife); // fade out as life → 1
    gl_Position = uVP * vec4(pos, 1.0);
}
