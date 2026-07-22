#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 2) in vec2 in_uv;

out vec2 v_uv;

void main()
{
    v_uv = in_uv;
    gl_Position = vec4(in_pos, 1.0);
}
