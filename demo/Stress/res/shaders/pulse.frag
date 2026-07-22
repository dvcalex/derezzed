#version 460 core

in vec2 v_uv;
flat in vec4 v_tint;

out vec4 frag_color;

void main() {
    float d = 1.0 - length(v_uv - 0.5);
    frag_color = v_tint * clamp(d, 0.0, 1.0);
}
