#version 460 core

layout(std430, binding = 0) buffer Positions 
{
    vec4 positions[];
};

layout(std430, binding = 2) buffer Colors
{
    vec4 colors[];
};

layout(location = 0) out vec4 color;
layout(location = 1) out float mass;

// Model View Projection Matrix
uniform mat4 mvp;

void main()
{
    vec4 pos = positions[gl_VertexID];
    gl_Position = mvp * vec4(pos.xyz, 1.0);
    gl_PointSize = 1.0;

    color = colors[gl_VertexID];
    mass = pos.w;
}