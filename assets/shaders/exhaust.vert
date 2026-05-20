#version 460 core

layout(location = 0) in vec3  iOrigin;
layout(location = 1) in float iSize;
layout(location = 2) in vec3  iDir;
layout(location = 3) in float iLength;
layout(location = 4) in float iThrust;

uniform mat4 uVP;
uniform vec3 uCamPos;
uniform vec3 uPlaneRight;

out vec2  vUV;
out float vThrust;

const vec2 CORNERS[6] = vec2[6](
    vec2(-1.0, 0.0),
    vec2(-1.0, 1.0),
    vec2( 1.0, 1.0),
    vec2(-1.0, 0.0),
    vec2( 1.0, 1.0),
    vec2( 1.0, 0.0)
);

void main()
{
    int quadIdx = gl_VertexID / 6;
    int localV  = gl_VertexID % 6;
    vec2 c = CORNERS[localV];

    vec3 up   = abs(iDir.y) < 0.9 ? vec3(0,1,0) : vec3(1,0,0);
    vec3 perp = normalize(cross(iDir, up));

    float angle = float(quadIdx) * (3.14159265 / 3.0);
    vec3 side   = perp * cos(angle) + cross(iDir, perp) * sin(angle);

    vec3 worldPos = iOrigin
                  + iDir * (c.y * iLength)
                  + side * (c.x * iSize * 3.0);

    gl_Position = uVP * vec4(worldPos, 1.0);
    vUV         = c;
    vThrust     = iThrust;
}
