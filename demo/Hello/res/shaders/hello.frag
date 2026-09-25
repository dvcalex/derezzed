#version 460 core

uniform sampler2D u_albedo;

in vec2 v_uv;

out vec4 fragColor;

void main()
{
    vec4 sampled = texture(u_albedo, v_uv.xy);
    fragColor = sampled * vec4(v_uv.xy, 0.0, 1.0);
}
