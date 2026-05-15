#version 460 core
in vec2 v_uv;
uniform vec4 u_tint;
out vec4 frag_color;
void main() {
    float d = 1.0 - length(v_uv - 0.5) * 2.0;
    frag_color = u_tint * clamp(d, 0.0, 1.0);
}
