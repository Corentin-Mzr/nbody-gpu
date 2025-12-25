#version 460 core

layout(location = 0) in vec4 color;
layout(location = 1) in float mass;

out vec4 frag_color;

void main() 
{
    // Antialiasing
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(uv, uv);
    
    float alpha = exp(-r2 * 2.5);

    // Output
    frag_color = vec4(color.rgb, alpha);
}
