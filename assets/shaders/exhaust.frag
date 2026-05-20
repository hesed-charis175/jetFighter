#version 460 core

in vec2  vUV;
in float vThrust;

out vec4 fragColor;

void main()
{
    float radial = abs(vUV.x);  
    float along  = vUV.y;      

float core   = pow(1.0 - radial, 3.0);
float sheath = 1.0 - smoothstep(0.0, 1.0, radial);
float shape  = core * 0.7 + sheath * 0.3; 
    vec3 colRoot = vec3(0.75, 0.95, 1.00);
    vec3 colMid  = vec3(1.00, 0.55, 0.05);
    vec3 colTip  = vec3(0.90, 0.15, 0.00);

    vec3 color;
    if (along < 0.35)
        color = mix(colRoot, colMid,  along / 0.35);
    else
        color = mix(colMid,  colTip, (along - 0.35) / 0.65);

    color = mix(vec3(1.0), color, smoothstep(0.0, 0.2, radial));

    float tipFade = 1.0 - smoothstep(0.7, 1.0, along);
    float alpha   = shape * tipFade * (0.5 + vThrust * 0.5);

    fragColor = vec4(color * alpha, alpha);
}
