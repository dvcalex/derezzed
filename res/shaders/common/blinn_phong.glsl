vec3 blinn_phong(vec3 normal, vec3 view_dir, vec3 light_dir, vec3 light_color, vec3 albedo, float shininess)
{
    // Assumes directions already point away from the surface and are normalized!
    vec3 diffuse = albedo * max(dot(light_dir, normal), 0.0);
    vec3 half_dir = normalize(light_dir + view_dir);
    vec3 specular = vec3(pow(max(dot(normal, half_dir), 0.0), shininess));
    return light_color * (diffuse + specular);
}
