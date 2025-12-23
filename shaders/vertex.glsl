#version 460 core

layout(std430, binding = 0) buffer Positions 
{
    vec4 positions[];
};

// Model View Projection Matrix
uniform mat4 mvp;

void main()
{
    vec4 pos = positions[gl_VertexID];
    gl_Position = mvp * vec4(pos.xyz, 1.0);
    gl_PointSize = 4.0;
}