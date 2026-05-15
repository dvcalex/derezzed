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
    float d = 1.0 - length(v_uv - 0.5);
    frag_color = draws[0].tint * clamp(d, 0.0, 1.0);
}
