#version 460 core

in vec2 v_uv;

out vec4 frag_color;

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
    frag_color = draws[0].tint;
}
