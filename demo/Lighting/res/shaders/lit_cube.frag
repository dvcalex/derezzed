#version 460 core

#include "common/blinn_phong.glsl"

in vec2 v_uv;
in vec3 v_normal;
in vec3 v_world_pos;

out vec4 out_color;

uniform vec3 u_camera_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_color;
uniform sampler2D u_albedo;

void main()
{
    // hardcoded material
    vec3 albedo = texture(u_albedo, v_uv).rgb;
    float shininess = 48.f;
    vec3 ambient_light_color = vec3(0.08f);

    // recall: interpolated normals are not guarenteed unit length
    vec3 normal = normalize(v_normal);
    vec3 view_dir = normalize(u_camera_pos - v_world_pos);
    vec3 light_dir = normalize(u_light_pos - v_world_pos);

    vec3 lit = blinn_phong(normal, view_dir, light_dir, u_light_color, albedo, shininess);
    vec3 color = ambient_light_color + lit;

    out_color = vec4(color, 1.f);
}
