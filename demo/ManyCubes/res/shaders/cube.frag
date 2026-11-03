#version 460 core

in vec3 v_normal;
in vec4 v_tint;

out vec4 out_color;

void main()
{
    vec3 n = normalize(v_normal);

    vec3 key_dir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 fill_dir = normalize(vec3(-0.6, 0.25, -0.4));
    float key = max(dot(n, key_dir), 0.0);
    float fill = max(dot(n, fill_dir), 0.0);

    vec3 lighting = vec3(0.18) // ambient
            + vec3(1.0, 0.85, 0.62) * key * 0.9 // warm term
            + vec3(0.35, 0.45, 0.7) * fill * 0.35; // cool term

    out_color = vec4(v_tint.rgb * lighting, 1.0);
}
