
#version 450 core

layout(set=0, binding=0) readonly buffer SSBO {
    mat4 models[];
} ssbo;

layout(set=1, binding=0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstantBlock {
    int index;
} pushConstant;

void main()
{
    gl_Position = camera.projection * camera.view * ssbo.models[pushConstant.index] * vec4(position, 1.0);
    fragColor = vec3(0.5, 0.5, 0.5);
    // fragColor = inColor;
}