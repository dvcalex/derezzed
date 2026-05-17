#version 460 core
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) in vec3 in_pos;
layout(location = 2) in vec2 in_uv;

out vec2 v_uv;
flat out vec4 v_tint;

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
    PerDraw d = draws[gl_BaseInstanceARB];
    gl_Position = vec4(in_pos.xy * d.scale + d.offset, 0.0, 1.0);
    v_uv = in_uv;
    v_tint = d.tint;
}
