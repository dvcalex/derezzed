#version 460 core

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

struct PerDraw {
    mat4 model;
};
layout(std430, binding = 0) buffer DrawData {
    PerDraw draws[];
};

uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_normal;

void main()
{
    mat4 model = draws[gl_BaseInstance].model;
    v_normal = mat3(transpose(inverse(model))) * in_normal;
    gl_Position = u_proj * u_view * model * vec4(in_pos, 1.0);
}
