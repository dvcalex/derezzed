#version 460 core

in vec2 v_uv;
flat in vec4 v_tint;

out vec4 frag_color;

void main() {
    frag_color = v_tint;
}
