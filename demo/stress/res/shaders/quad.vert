#version 460 core
layout(location = 0) in vec3 a_pos;
layout(location = 2) in vec2 a_uv;

uniform vec2  u_offset;
uniform float u_scale;

out vec2 v_uv;

void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos.xy * u_scale + u_offset, 0.0, 1.0);
}
