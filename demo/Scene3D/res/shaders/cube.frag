#version 460 core

in vec3 v_normal;
out vec4 out_color;

void main()
{
    vec3 n = normalize(v_normal);
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(n, light_dir), 0.0); // basic phong N dot L lambert lighting
    vec3 base = vec3(0.7, 0.5, 0.9);
    out_color = vec4(base * (0.15 + 0.85 * diff), 1.0);
}
