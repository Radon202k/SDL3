#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

layout(set = 1, binding = 0) uniform UniformBufferObject
{
    layout(row_major) mat4 projection;
} ubo;

void main()
{
    gl_Position = ubo.projection * vec4(inPosition, 0.0, 1.0);
    outUV = inUV;
    outColor = inColor;
}