#version 460 core

in vec3 v_normal;
in vec3 v_world_pos;
in vec4 v_tint;

uniform vec3 u_cam_pos;
uniform vec3 u_fog_color;
uniform float u_fog_near;
uniform float u_fog_far;

out vec4 out_color;

void main()
{
    vec3 n = normalize(v_normal);

    // two-light setup: warm key + cool fill so the grid reads with depth even in shadowed faces
    vec3 key_dir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 fill_dir = normalize(vec3(-0.6, 0.25, -0.4));
    float key = max(dot(n, key_dir), 0.0);
    float fill = max(dot(n, fill_dir), 0.0);

    vec3 lighting = vec3(0.18) // ambient
            + vec3(1.0, 0.85, 0.62) * key * 0.9 // warm sun
            + vec3(0.35, 0.45, 0.7) * fill * 0.35; // cool sky fill

    vec3 color = v_tint.rgb * lighting;

    // distance fog blends into the clear color so far rows melt into the horizon
    float dist = length(v_world_pos - u_cam_pos);
    float fog = clamp((dist - u_fog_near) / (u_fog_far - u_fog_near), 0.0, 1.0);
    color = mix(color, u_fog_color, fog);

    out_color = vec4(color, 1.0);
}
