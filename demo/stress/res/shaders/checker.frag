#version 460 core

in vec2 v_uv;
flat in vec4 v_tint;

out vec4 frag_color;

void main() {
    vec2 g = step(0.5, fract(v_uv * 2.0));
    float c = mod(g.x + g.y, 2.0);
    frag_color = v_tint * (0.4 + 0.6 * c);
}
