#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 2) in vec2 in_uv;

out vec2 v_uv;

struct PerDraw {
    vec2 offset;
    float scale;
    float _pad;
    vec4 tint;
};
layout(std430, binding = 0) readonly buffer Draws {
    PerDraw draws[];
};

void main() {
    PerDraw d = draws[0];
    gl_Position = vec4(in_pos.xy * d.scale + d.offset, 0.0, 1.0);
    v_uv = in_uv;
}
