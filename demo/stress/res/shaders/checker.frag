#version 460 core
in vec2 v_uv;
uniform vec4 u_tint;
out vec4 frag_color;
void main() {
    vec2 g = step(0.5, fract(v_uv * 2.0));
    float c = mod(g.x + g.y, 2.0);
    frag_color = u_tint * (0.4 + 0.6 * c);
}
