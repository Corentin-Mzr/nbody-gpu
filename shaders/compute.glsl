#version 430 core

layout(local_size_x = 128) in;

layout(std430, binding = 0) buffer PositionsIn 
{
    vec4 positions_and_masses_in[];
};

layout(std430, binding = 1) buffer Velocities 
{
    vec4 velocities[];
}; 

layout(std430, binding = 2) buffer PositionsOut
{
    vec4 positions_and_masses_out[];
};

shared vec4 local_positions_and_masses_in[128];

uniform uint count;
uniform float dt;
uniform float gravity;
uniform float distance_threshold;
uniform uint iter_per_frame;

// vec4 compute_acceleration(uint gid)
// {
//     vec4 acceleration = vec4(0.0);
    
//     for (uint i = 0; i < count; ++i)
//     {
//         if (i == gid)
//         {
//             continue;
//         }

//         vec4 dpos = positions[i] - positions[gid];
//         float distance_sq = dot(dpos, dpos);

//         if (distance_sq < distance_threshold * distance_threshold)
//         {
//             continue;
//         }

//         float inv_r3 = 1.0 / (distance_sq * sqrt(distance_sq));
//         acceleration += gravity * masses[i] * dpos * inv_r3;
//     }

//     return acceleration;
// }

vec3 compute_acceleration(vec3 position, uint gid, uint tile_size)
{
    vec3 acceleration = vec3(0.0);
    uint num_tiles = (count + tile_size - 1) / tile_size;
    
    for (uint tile = 0; tile < num_tiles; ++tile)
    {
        uint tid = gl_LocalInvocationID.x;
        uint idx = tile * tile_size + tid;

        if (idx < count)
        {
            local_positions_and_masses_in[tid] = positions_and_masses_in[idx];
        }
        barrier();

        uint tile_end = min(tile_size, count - tile * tile_size);
        for (uint j = 0; j < tile_end; ++j)
        {
            uint i = tile * tile_size + j;
            if (i == gid)
            {
                continue;
            }

            vec3 dpos = local_positions_and_masses_in[j].xyz - position;
            float distance_sq = dot(dpos, dpos);

            if (distance_sq < distance_threshold * distance_threshold)
            {
                continue;
            }

            float inv_r = inversesqrt(distance_sq);
            float inv_r3 = inv_r * inv_r * inv_r;
            acceleration += gravity * local_positions_and_masses_in[j].w * dpos * inv_r3;

        }
        barrier();
    }

    return acceleration;
}

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= count)
    {
        return;
    }

    vec3 position = positions_and_masses_in[gid].xyz;
    float mass = positions_and_masses_in[gid].w;
    vec3 velocity = velocities[gid].xyz;

    for (uint i = 0; i < iter_per_frame; ++i)
    {
        vec3 acceleration = compute_acceleration(position, gid, 256);
        velocity += acceleration * dt;
        position += velocity * dt;
    }

    velocities[gid] = vec4(velocity, 0.0);
    positions_and_masses_out[gid] = vec4(position, mass);
}