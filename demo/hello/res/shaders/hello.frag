#version 460 core

in vec2 v_uv;

out vec4 fragColor;

void main()
{
    fragColor = vec4(v_uv.xy, 1.0, 1.0);
}
