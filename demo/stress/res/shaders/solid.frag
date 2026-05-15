#version 460 core
uniform vec4 u_tint;
out vec4 frag_color;
void main() {
    frag_color = u_tint;
}
